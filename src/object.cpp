#include "pcheader.h"
#include "vm.h"
#include "lvstring.h"
#include "array.h"
#include "table.h"
#include "userdata.h"
#include "funcproto.h"
#include "class.h"
#include "closure.h"

#define CLOSURESTREAM_HEAD (('S'<<24)|('Q'<<16)|('I'<<8)|('R'))
#define CLOSURESTREAM_PART (('P'<<24)|('A'<<16)|('R'<<8)|('T'))
#define CLOSURESTREAM_TAIL (('T'<<24)|('A'<<16)|('I'<<8)|('L'))

const LVChar *IdType2Name(LVObjectType type) {
	switch (_RAW_TYPE(type)) {
		case _RT_NULL:
			return _LC("null");
		case _RT_INTEGER:
			return _LC("integer");
		case _RT_FLOAT:
			return _LC("float");
		case _RT_BOOL:
			return _LC("bool");
		case _RT_STRING:
			return _LC("string");
		case _RT_TABLE:
			return _LC("table");
		case _RT_ARRAY:
			return _LC("array");
		case _RT_GENERATOR:
			return _LC("generator");
		case _RT_CLOSURE:
		case _RT_NATIVECLOSURE:
			return _LC("function");
		case _RT_USERDATA:
		case _RT_USERPOINTER:
			return _LC("userdata");
		case _RT_THREAD:
			return _LC("thread");
		case _RT_FUNCPROTO:
			return _LC("function");
		case _RT_CLASS:
			return _LC("class");
		case _RT_INSTANCE:
			return _LC("instance");
		case _RT_WEAKREF:
			return _LC("weakref");
		case _RT_OUTER:
			return _LC("outer");
		default:
			return NULL;
	}
}

const LVChar *GetTypeName(const LVObjectPtr& obj1) {
	return IdType2Name(type(obj1));
}

LVString *LVString::Create(LVSharedState *ss, const LVChar *s, LVInteger len) {
	return ADD_STRING(ss, s, len);
}

void LVString::Release() {
	REMOVE_STRING(_sharedstate, this);
}

LVInteger LVString::Next(const LVObjectPtr& refpos, LVObjectPtr& outkey, LVObjectPtr& outval) {
	LVInteger idx = (LVInteger)TranslateIndex(refpos);
	while (idx < _len) {
		outkey = (LVInteger)idx;
		outval = (LVInteger)((LVUnsignedInteger)_val[idx]);
		//return idx for the next iteration
		return ++idx;
	}
	//nothing to iterate anymore
	return -1;
}

LVUnsignedInteger TranslateIndex(const LVObjectPtr& idx) {
	switch (type(idx)) {
		case OT_NULL:
			return 0;
		case OT_INTEGER:
			return (LVUnsignedInteger)_integer(idx);
		default:
			assert(0);
			break;
	}
	return 0;
}

LVWeakRef *LVRefCounted::GetWeakRef(LVObjectType type) {
	if (!_weakref) {
		lv_new(_weakref, LVWeakRef);
#if defined(USEDOUBLE) && !defined(_LV64)
		_weakref->_obj._unVal.raw = 0; //clean the whole union on 32 bits with double
#endif
		_weakref->_obj._type = type;
		_weakref->_obj._unVal.pRefCounted = this;
	}
	return _weakref;
}

LVRefCounted::~LVRefCounted() {
	if (_weakref) {
		_weakref->_obj._type = OT_NULL;
		_weakref->_obj._unVal.pRefCounted = NULL;
	}
}

void LVWeakRef::Release() {
	if (ISREFCOUNTED(_obj._type)) {
		_obj._unVal.pRefCounted->_weakref = NULL;
	}
	lv_delete(this, LVWeakRef);
}

bool LVDelegable::GetMetaMethod(LVVM *v, LVMetaMethod mm, LVObjectPtr& res) {
	if (_delegate) {
		return _delegate->Get((*_ss(v)->_metamethods)[mm], res);
	}
	return false;
}

bool LVDelegable::SetDelegate(LVTable *mt) {
	LVTable *temp = mt;
	if (temp == this) return false;
	while (temp) {
		if (temp->_delegate == this) return false; //cycle detected
		temp = temp->_delegate;
	}
	if (mt) __ObjAddRef(mt);
	__ObjRelease(_delegate);
	_delegate = mt;
	return true;
}

bool LVGenerator::Yield(LVVM *v, LVInteger target) {
	if (_state == eSuspended) {
		v->Raise_Error(_LC("internal vm error, yielding dead generator"));
		return false;
	}
	if (_state == eDead) {
		v->Raise_Error(_LC("internal vm error, yielding a dead generator"));
		return false;
	}
	LVInteger size = v->_top - v->_stackbase;

	_stack.resize(size);
	LVObject _this = v->_stack[v->_stackbase];
	_stack._vals[0] = ISREFCOUNTED(type(_this)) ? LVObjectPtr(_refcounted(_this)->GetWeakRef(type(_this))) : _this;
	for (LVInteger n = 1; n < target; n++) {
		_stack._vals[n] = v->_stack[v->_stackbase + n];
	}
	for (LVInteger j = 0; j < size; j++) {
		v->_stack[v->_stackbase + j].Null();
	}

	_ci = *v->ci;
	_ci._generator = NULL;
	for (LVInteger i = 0; i < _ci._etraps; i++) {
		_etraps.push_back(v->_etraps.top());
		v->_etraps.pop_back();
		// store relative stack base and size in case of resume to other _top
		LVExceptionTrap& et = _etraps.back();
		et._stackbase -= v->_stackbase;
		et._stacksize -= v->_stackbase;
	}
	_state = eSuspended;
	return true;
}

bool LVGenerator::Resume(LVVM *v, LVObjectPtr& dest) {
	if (_state == eDead) {
		v->Raise_Error(_LC("resuming dead generator"));
		return false;
	}
	if (_state == eRunning) {
		v->Raise_Error(_LC("resuming active generator"));
		return false;
	}
	LVInteger size = _stack.size();
	LVInteger target = &dest - &(v->_stack._vals[v->_stackbase]);
	assert(target >= 0 && target <= 255);
	LVInteger newbase = v->_top;
	if (!v->EnterFrame(v->_top, v->_top + size, false))
		return false;
	v->ci->_generator   = this;
	v->ci->_target      = (LVInt32)target;
	v->ci->_closure     = _ci._closure;
	v->ci->_ip          = _ci._ip;
	v->ci->_literals    = _ci._literals;
	v->ci->_ncalls      = _ci._ncalls;
	v->ci->_etraps      = _ci._etraps;
	v->ci->_root        = _ci._root;


	for (LVInteger i = 0; i < _ci._etraps; i++) {
		v->_etraps.push_back(_etraps.top());
		_etraps.pop_back();
		LVExceptionTrap& et = v->_etraps.back();
		// restore absolute stack base and size
		et._stackbase += newbase;
		et._stacksize += newbase;
	}
	LVObject _this = _stack._vals[0];
	v->_stack[v->_stackbase] = type(_this) == OT_WEAKREF ? _weakref(_this)->_obj : _this;

	for (LVInteger n = 1; n < size; n++) {
		v->_stack[v->_stackbase + n] = _stack._vals[n];
		_stack._vals[n].Null();
	}

	_state = eRunning;
	if (v->_debughook)
		v->CallDebugHook(_LC('c'));

	return true;
}

void LVArray::Extend(const LVArray *a) {
	LVInteger xlen;
	if ((xlen = a->Size()))
		for (LVInteger i = 0; i < xlen; i++)
			Append(a->_values[i]);
}

const LVChar *FunctionPrototype::GetLocal(LVVM *vm, LVUnsignedInteger stackbase, LVUnsignedInteger nseq, LVUnsignedInteger nop) {
	LVUnsignedInteger nvars = _nlocalvarinfos;
	const LVChar *res = NULL;
	if (nvars >= nseq) {
		for (LVUnsignedInteger i = 0; i < nvars; i++) {
			if (_localvarinfos[i]._start_op <= nop && _localvarinfos[i]._end_op >= nop) {
				if (nseq == 0) {
					vm->Push(vm->_stack[stackbase + _localvarinfos[i]._pos]);
					res = _stringval(_localvarinfos[i]._name);
					break;
				}
				nseq--;
			}
		}
	}
	return res;
}


LVInteger FunctionPrototype::GetLine(LVInstruction *curr) {
	LVInteger op = (LVInteger)(curr - _instructions);
	LVInteger line = _lineinfos[0]._line;
	LVInteger low = 0;
	LVInteger high = _nlineinfos - 1;
	LVInteger mid = 0;
	while (low <= high) {
		mid = low + ((high - low) >> 1);
		LVInteger curop = _lineinfos[mid]._op;
		if (curop > op) {
			high = mid - 1;
		} else if (curop < op) {
			if (mid < (_nlineinfos - 1)
			        && _lineinfos[mid + 1]._op >= op) {
				break;
			}
			low = mid + 1;
		} else { //equal
			break;
		}
	}

	while (mid > 0 && _lineinfos[mid]._op >= op) mid--;

	line = _lineinfos[mid]._line;

	return line;
}

LVClosure::~LVClosure() {
	__ObjRelease(_root);
	__ObjRelease(_env);
	__ObjRelease(_base);
	REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain, this);
}

#define _CHECK_IO(exp) { \
	if (!exp) \
		return false; \
}

bool SafeWrite(VMHANDLE v, LVWRITEFUNC write, LVUserPointer up, LVUserPointer dest, LVInteger size) {
	if (write(up, dest, size) != size) {
		v->Raise_Error(_LC("io error (write function failure)"));
		return false;
	}
	return true;
}

bool SafeRead(VMHANDLE v, LVWRITEFUNC read, LVUserPointer up, LVUserPointer dest, LVInteger size) {
	if (size && read(up, dest, size) != size) {
		v->Raise_Error(_LC("io error, read function failure, the origin stream could be corrupted/trucated"));
		return false;
	}
	return true;
}

bool WriteTag(VMHANDLE v, LVWRITEFUNC write, LVUserPointer up, LVUnsignedInteger32 tag) {
	return SafeWrite(v, write, up, &tag, sizeof(tag));
}

bool CheckTag(VMHANDLE v, LVWRITEFUNC read, LVUserPointer up, LVUnsignedInteger32 tag) {
	LVUnsignedInteger32 t;
	_CHECK_IO(SafeRead(v, read, up, &t, sizeof(t)));
	if (t != tag) {
		v->Raise_Error(_LC("invalid or corrupted closure stream"));
		return false;
	}
	return true;
}

bool WriteObject(VMHANDLE v, LVUserPointer up, LVWRITEFUNC write, LVObjectPtr& o) {
	LVUnsignedInteger32 _type = (LVUnsignedInteger32)type(o);
	_CHECK_IO(SafeWrite(v, write, up, &_type, sizeof(_type)));
	switch (type(o)) {
		case OT_STRING:
			_CHECK_IO(SafeWrite(v, write, up, &_string(o)->_len, sizeof(LVInteger)));
			_CHECK_IO(SafeWrite(v, write, up, _stringval(o), lv_rsl(_string(o)->_len)));
			break;
		case OT_BOOL:
		case OT_INTEGER:
			_CHECK_IO(SafeWrite(v, write, up, &_integer(o), sizeof(LVInteger)));
			break;
		case OT_FLOAT:
			_CHECK_IO(SafeWrite(v, write, up, &_float(o), sizeof(LVFloat)));
			break;
		case OT_NULL:
			break;
		default:
			v->Raise_Error(_LC("cannot serialize a %s"), GetTypeName(o));
			return false;
	}
	return true;
}

bool ReadObject(VMHANDLE v, LVUserPointer up, LVREADFUNC read, LVObjectPtr& o) {
	LVUnsignedInteger32 _type;
	_CHECK_IO(SafeRead(v, read, up, &_type, sizeof(_type)));
	LVObjectType t = (LVObjectType)_type;
	switch (t) {
		case OT_STRING: {
			LVInteger len;
			_CHECK_IO(SafeRead(v, read, up, &len, sizeof(LVInteger)));
			_CHECK_IO(SafeRead(v, read, up, _ss(v)->GetScratchPad(lv_rsl(len)), lv_rsl(len)));
			o = LVString::Create(_ss(v), _ss(v)->GetScratchPad(-1), len);
		}
		break;
		case OT_INTEGER: {
			LVInteger i;
			_CHECK_IO(SafeRead(v, read, up, &i, sizeof(LVInteger)));
			o = i;
			break;
		}
		case OT_BOOL: {
			LVInteger i;
			_CHECK_IO(SafeRead(v, read, up, &i, sizeof(LVInteger)));
			o._type = OT_BOOL;
			o._unVal.nInteger = i;
			break;
		}
		case OT_FLOAT: {
			LVFloat f;
			_CHECK_IO(SafeRead(v, read, up, &f, sizeof(LVFloat)));
			o = f;
			break;
		}
		case OT_NULL:
			o.Null();
			break;
		default:
			v->Raise_Error(_LC("cannot serialize a %s"), IdType2Name(t));
			return false;
	}
	return true;
}

bool LVClosure::Save(LVVM *v, LVUserPointer up, LVWRITEFUNC write) {
	_CHECK_IO(WriteTag(v, write, up, CLOSURESTREAM_HEAD));
	_CHECK_IO(WriteTag(v, write, up, sizeof(LVChar)));
	_CHECK_IO(WriteTag(v, write, up, sizeof(LVInteger)));
	_CHECK_IO(WriteTag(v, write, up, sizeof(LVFloat)));
	_CHECK_IO(_function->Save(v, up, write));
	_CHECK_IO(WriteTag(v, write, up, CLOSURESTREAM_TAIL));
	return true;
}

bool LVClosure::Load(LVVM *v, LVUserPointer up, LVREADFUNC read, LVObjectPtr& ret) {
	_CHECK_IO(CheckTag(v, read, up, CLOSURESTREAM_HEAD));
	_CHECK_IO(CheckTag(v, read, up, sizeof(LVChar)));
	_CHECK_IO(CheckTag(v, read, up, sizeof(LVInteger)));
	_CHECK_IO(CheckTag(v, read, up, sizeof(LVFloat)));
	LVObjectPtr func;
	_CHECK_IO(FunctionPrototype::Load(v, up, read, func));
	_CHECK_IO(CheckTag(v, read, up, CLOSURESTREAM_TAIL));
	ret = LVClosure::Create(_ss(v), _funcproto(func), _table(v->_roottable)->GetWeakRef(OT_TABLE));
	//FIXME: load an root for this closure
	return true;
}

FunctionPrototype::FunctionPrototype(LVSharedState *ss) {
	_stacksize = 0;
	_bgenerator = false;
	INIT_CHAIN();
	ADD_TO_CHAIN(&_ss(this)->_gc_chain, this);
}

FunctionPrototype::~FunctionPrototype() {
	REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain, this);
}

bool FunctionPrototype::Save(LVVM *v, LVUserPointer up, LVWRITEFUNC write) {
	LVInteger i, nliterals = _nliterals, nparameters = _nparameters;
	LVInteger noutervalues = _noutervalues, nlocalvarinfos = _nlocalvarinfos;
	LVInteger nlineinfos = _nlineinfos, ninstructions = _ninstructions, nfunctions = _nfunctions;
	LVInteger ndefaultparams = _ndefaultparams;
	_CHECK_IO(WriteTag(v, write, up, CLOSURESTREAM_PART));
	_CHECK_IO(WriteObject(v, up, write, _sourcename));
	_CHECK_IO(WriteObject(v, up, write, _name));
	_CHECK_IO(WriteTag(v, write, up, CLOSURESTREAM_PART));
	_CHECK_IO(SafeWrite(v, write, up, &nliterals, sizeof(nliterals)));
	_CHECK_IO(SafeWrite(v, write, up, &nparameters, sizeof(nparameters)));
	_CHECK_IO(SafeWrite(v, write, up, &noutervalues, sizeof(noutervalues)));
	_CHECK_IO(SafeWrite(v, write, up, &nlocalvarinfos, sizeof(nlocalvarinfos)));
	_CHECK_IO(SafeWrite(v, write, up, &nlineinfos, sizeof(nlineinfos)));
	_CHECK_IO(SafeWrite(v, write, up, &ndefaultparams, sizeof(ndefaultparams)));
	_CHECK_IO(SafeWrite(v, write, up, &ninstructions, sizeof(ninstructions)));
	_CHECK_IO(SafeWrite(v, write, up, &nfunctions, sizeof(nfunctions)));
	_CHECK_IO(WriteTag(v, write, up, CLOSURESTREAM_PART));
	for (i = 0; i < nliterals; i++) {
		_CHECK_IO(WriteObject(v, up, write, _literals[i]));
	}

	_CHECK_IO(WriteTag(v, write, up, CLOSURESTREAM_PART));
	for (i = 0; i < nparameters; i++) {
		_CHECK_IO(WriteObject(v, up, write, _parameters[i]));
	}

	_CHECK_IO(WriteTag(v, write, up, CLOSURESTREAM_PART));
	for (i = 0; i < noutervalues; i++) {
		_CHECK_IO(SafeWrite(v, write, up, &_outervalues[i]._type, sizeof(LVUnsignedInteger)));
		_CHECK_IO(WriteObject(v, up, write, _outervalues[i]._src));
		_CHECK_IO(WriteObject(v, up, write, _outervalues[i]._name));
	}

	_CHECK_IO(WriteTag(v, write, up, CLOSURESTREAM_PART));
	for (i = 0; i < nlocalvarinfos; i++) {
		LVLocalVarInfo& lvi = _localvarinfos[i];
		_CHECK_IO(WriteObject(v, up, write, lvi._name));
		_CHECK_IO(SafeWrite(v, write, up, &lvi._pos, sizeof(LVUnsignedInteger)));
		_CHECK_IO(SafeWrite(v, write, up, &lvi._start_op, sizeof(LVUnsignedInteger)));
		_CHECK_IO(SafeWrite(v, write, up, &lvi._end_op, sizeof(LVUnsignedInteger)));
	}

	_CHECK_IO(WriteTag(v, write, up, CLOSURESTREAM_PART));
	_CHECK_IO(SafeWrite(v, write, up, _lineinfos, sizeof(LVLineInfo)*nlineinfos));

	_CHECK_IO(WriteTag(v, write, up, CLOSURESTREAM_PART));
	_CHECK_IO(SafeWrite(v, write, up, _defaultparams, sizeof(LVInteger)*ndefaultparams));

	_CHECK_IO(WriteTag(v, write, up, CLOSURESTREAM_PART));
	_CHECK_IO(SafeWrite(v, write, up, _instructions, sizeof(LVInstruction)*ninstructions));

	_CHECK_IO(WriteTag(v, write, up, CLOSURESTREAM_PART));
	for (i = 0; i < nfunctions; i++) {
		_CHECK_IO(_funcproto(_functions[i])->Save(v, up, write));
	}
	_CHECK_IO(SafeWrite(v, write, up, &_stacksize, sizeof(_stacksize)));
	_CHECK_IO(SafeWrite(v, write, up, &_bgenerator, sizeof(_bgenerator)));
	_CHECK_IO(SafeWrite(v, write, up, &_varparams, sizeof(_varparams)));
	return true;
}

bool FunctionPrototype::Load(LVVM *v, LVUserPointer up, LVREADFUNC read, LVObjectPtr& ret) {
	LVInteger i, nliterals, nparameters;
	LVInteger noutervalues , nlocalvarinfos ;
	LVInteger nlineinfos, ninstructions , nfunctions, ndefaultparams ;
	LVObjectPtr sourcename, name;
	LVObjectPtr o;
	_CHECK_IO(CheckTag(v, read, up, CLOSURESTREAM_PART));
	_CHECK_IO(ReadObject(v, up, read, sourcename));
	_CHECK_IO(ReadObject(v, up, read, name));

	_CHECK_IO(CheckTag(v, read, up, CLOSURESTREAM_PART));
	_CHECK_IO(SafeRead(v, read, up, &nliterals, sizeof(nliterals)));
	_CHECK_IO(SafeRead(v, read, up, &nparameters, sizeof(nparameters)));
	_CHECK_IO(SafeRead(v, read, up, &noutervalues, sizeof(noutervalues)));
	_CHECK_IO(SafeRead(v, read, up, &nlocalvarinfos, sizeof(nlocalvarinfos)));
	_CHECK_IO(SafeRead(v, read, up, &nlineinfos, sizeof(nlineinfos)));
	_CHECK_IO(SafeRead(v, read, up, &ndefaultparams, sizeof(ndefaultparams)));
	_CHECK_IO(SafeRead(v, read, up, &ninstructions, sizeof(ninstructions)));
	_CHECK_IO(SafeRead(v, read, up, &nfunctions, sizeof(nfunctions)));


	FunctionPrototype *f = FunctionPrototype::Create(_opt_ss(v), ninstructions, nliterals, nparameters,
	                       nfunctions, noutervalues, nlineinfos, nlocalvarinfos, ndefaultparams);
	LVObjectPtr proto = f; //gets a ref in case of failure
	f->_sourcename = sourcename;
	f->_name = name;

	_CHECK_IO(CheckTag(v, read, up, CLOSURESTREAM_PART));

	for (i = 0; i < nliterals; i++) {
		_CHECK_IO(ReadObject(v, up, read, o));
		f->_literals[i] = o;
	}
	_CHECK_IO(CheckTag(v, read, up, CLOSURESTREAM_PART));

	for (i = 0; i < nparameters; i++) {
		_CHECK_IO(ReadObject(v, up, read, o));
		f->_parameters[i] = o;
	}
	_CHECK_IO(CheckTag(v, read, up, CLOSURESTREAM_PART));

	for (i = 0; i < noutervalues; i++) {
		LVUnsignedInteger type;
		LVObjectPtr name;
		_CHECK_IO(SafeRead(v, read, up, &type, sizeof(LVUnsignedInteger)));
		_CHECK_IO(ReadObject(v, up, read, o));
		_CHECK_IO(ReadObject(v, up, read, name));
		f->_outervalues[i] = LVOuterVar(name, o, (LVOuterType)type);
	}
	_CHECK_IO(CheckTag(v, read, up, CLOSURESTREAM_PART));

	for (i = 0; i < nlocalvarinfos; i++) {
		LVLocalVarInfo lvi;
		_CHECK_IO(ReadObject(v, up, read, lvi._name));
		_CHECK_IO(SafeRead(v, read, up, &lvi._pos, sizeof(LVUnsignedInteger)));
		_CHECK_IO(SafeRead(v, read, up, &lvi._start_op, sizeof(LVUnsignedInteger)));
		_CHECK_IO(SafeRead(v, read, up, &lvi._end_op, sizeof(LVUnsignedInteger)));
		f->_localvarinfos[i] = lvi;
	}
	_CHECK_IO(CheckTag(v, read, up, CLOSURESTREAM_PART));
	_CHECK_IO(SafeRead(v, read, up, f->_lineinfos, sizeof(LVLineInfo)*nlineinfos));

	_CHECK_IO(CheckTag(v, read, up, CLOSURESTREAM_PART));
	_CHECK_IO(SafeRead(v, read, up, f->_defaultparams, sizeof(LVInteger)*ndefaultparams));

	_CHECK_IO(CheckTag(v, read, up, CLOSURESTREAM_PART));
	_CHECK_IO(SafeRead(v, read, up, f->_instructions, sizeof(LVInstruction)*ninstructions));

	_CHECK_IO(CheckTag(v, read, up, CLOSURESTREAM_PART));
	for (i = 0; i < nfunctions; i++) {
		_CHECK_IO(_funcproto(o)->Load(v, up, read, o));
		f->_functions[i] = o;
	}
	_CHECK_IO(SafeRead(v, read, up, &f->_stacksize, sizeof(f->_stacksize)));
	_CHECK_IO(SafeRead(v, read, up, &f->_bgenerator, sizeof(f->_bgenerator)));
	_CHECK_IO(SafeRead(v, read, up, &f->_varparams, sizeof(f->_varparams)));

	ret = f;
	return true;
}

#ifndef NO_GARBAGE_COLLECTOR

#define START_MARK()    if(!(_uiRef&MARK_FLAG)){ \
		_uiRef|=MARK_FLAG;

#define END_MARK() RemoveFromChain(&_sharedstate->_gc_chain, this); \
		AddToChain(chain, this); }

void LVVM::Mark(LVCollectable **chain) {
	START_MARK()
	LVSharedState::MarkObject(_lasterror, chain);
	LVSharedState::MarkObject(_errorhandler, chain);
	LVSharedState::MarkObject(_debughook_closure, chain);
	LVSharedState::MarkObject(_roottable, chain);
	LVSharedState::MarkObject(temp_reg, chain);
	for (LVUnsignedInteger i = 0; i < _stack.size(); i++) LVSharedState::MarkObject(_stack[i], chain);
	for (LVInteger k = 0; k < _callsstacksize; k++) LVSharedState::MarkObject(_callsstack[k]._closure, chain);
	END_MARK()
}

void LVArray::Mark(LVCollectable **chain) {
	START_MARK()
	LVInteger len = _values.size();
	for (LVInteger i = 0; i < len; i++) LVSharedState::MarkObject(_values[i], chain);
	END_MARK()
}
void LVTable::Mark(LVCollectable **chain) {
	START_MARK()
	if (_delegate) _delegate->Mark(chain);
	LVInteger len = _numofnodes;
	for (LVInteger i = 0; i < len; i++) {
		LVSharedState::MarkObject(_nodes[i].key, chain);
		LVSharedState::MarkObject(_nodes[i].val, chain);
	}
	END_MARK()
}

void LVClass::Mark(LVCollectable **chain) {
	START_MARK()
	_members->Mark(chain);
	if (_base) _base->Mark(chain);
	LVSharedState::MarkObject(_attributes, chain);
	for (LVUnsignedInteger i = 0; i < _defaultvalues.size(); i++) {
		LVSharedState::MarkObject(_defaultvalues[i].val, chain);
		LVSharedState::MarkObject(_defaultvalues[i].attrs, chain);
	}
	for (LVUnsignedInteger j = 0; j < _methods.size(); j++) {
		LVSharedState::MarkObject(_methods[j].val, chain);
		LVSharedState::MarkObject(_methods[j].attrs, chain);
	}
	for (LVUnsignedInteger k = 0; k < MT_LAST; k++) {
		LVSharedState::MarkObject(_metamethods[k], chain);
	}
	END_MARK()
}

void LVInstance::Mark(LVCollectable **chain) {
	START_MARK()
	_class->Mark(chain);
	LVUnsignedInteger nvalues = _class->_defaultvalues.size();
	for (LVUnsignedInteger i = 0; i < nvalues; i++) {
		LVSharedState::MarkObject(_values[i], chain);
	}
	END_MARK()
}

void LVGenerator::Mark(LVCollectable **chain) {
	START_MARK()
	for (LVUnsignedInteger i = 0; i < _stack.size(); i++) LVSharedState::MarkObject(_stack[i], chain);
	LVSharedState::MarkObject(_closure, chain);
	END_MARK()
}

void FunctionPrototype::Mark(LVCollectable **chain) {
	START_MARK()
	for (LVInteger i = 0; i < _nliterals; i++) LVSharedState::MarkObject(_literals[i], chain);
	for (LVInteger k = 0; k < _nfunctions; k++) LVSharedState::MarkObject(_functions[k], chain);
	END_MARK()
}

void LVClosure::Mark(LVCollectable **chain) {
	START_MARK()
	if (_base) _base->Mark(chain);
	FunctionPrototype *fp = _function;
	fp->Mark(chain);
	for (LVInteger i = 0; i < fp->_noutervalues; i++) LVSharedState::MarkObject(_outervalues[i], chain);
	for (LVInteger k = 0; k < fp->_ndefaultparams; k++) LVSharedState::MarkObject(_defaultparams[k], chain);
	END_MARK()
}

void LVNativeClosure::Mark(LVCollectable **chain) {
	START_MARK()
	for (LVUnsignedInteger i = 0; i < _noutervalues; i++) LVSharedState::MarkObject(_outervalues[i], chain);
	END_MARK()
}

void LVOuter::Mark(LVCollectable **chain) {
	START_MARK()
	/* If the valptr points to a closed value, that value is alive */
	if (_valptr == &_value) {
		LVSharedState::MarkObject(_value, chain);
	}
	END_MARK()
}

void LVUserData::Mark(LVCollectable **chain) {
	START_MARK()
	if (_delegate) _delegate->Mark(chain);
	END_MARK()
}

void LVCollectable::UnMark() {
	_uiRef &= ~MARK_FLAG;
}

#endif

#ifdef _DEBUG
void LVObjectPtr::dump() {
	scprintf(_LC("DELEGABLE %s\n"), is_delegable(*this) ? "true" : "false");
	switch (type(*this)) {
		case OT_FLOAT:
			scprintf(_LC("FLOAT %.3f"), _float(*this));
			break;
		case OT_INTEGER:
			scprintf(_LC("INTEGER " _PRINT_INT_FMT), _integer(*this));
			break;
		case OT_BOOL:
			scprintf(_LC("BOOL %s"), _integer(*this) ? "true" : "false");
			break;
		case OT_STRING:
			scprintf(_LC("STRING %s"), _stringval(*this));
			break;
		case OT_NULL:
			scprintf(_LC("NULL"));
			break;
		case OT_TABLE:
			scprintf(_LC("TABLE %p[%p]"), _table(*this), _table(*this)->_delegate);
			break;
		case OT_ARRAY:
			scprintf(_LC("ARRAY %p"), _array(*this));
			break;
		case OT_CLOSURE:
			scprintf(_LC("CLOSURE [%p]"), _closure(*this));
			break;
		case OT_NATIVECLOSURE:
			scprintf(_LC("NATIVECLOSURE"));
			break;
		case OT_USERDATA:
			scprintf(_LC("USERDATA %p[%p]"), _userdataval(*this), _userdata(*this)->_delegate);
			break;
		case OT_GENERATOR:
			scprintf(_LC("GENERATOR %p"), _generator(*this));
			break;
		case OT_THREAD:
			scprintf(_LC("THREAD [%p]"), _thread(*this));
			break;
		case OT_USERPOINTER:
			scprintf(_LC("USERPOINTER %p"), _userpointer(*this));
			break;
		case OT_CLASS:
			scprintf(_LC("CLASS %p"), _class(*this));
			break;
		case OT_INSTANCE:
			scprintf(_LC("INSTANCE %p"), _instance(*this));
			break;
		case OT_WEAKREF:
			scprintf(_LC("WEAKERF %p"), _weakref(*this));
			break;
		default:
			assert(0);
			break;
	};
	scprintf(_LC("\n"));
}
#endif
