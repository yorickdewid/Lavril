#include "pcheader.h"
#ifndef NO_COMPILER
#include "compiler.h"
#include "lvstring.h"
#include "funcproto.h"
#include "table.h"
#include "opcodes.h"
#include "funcstate.h"

#ifdef _DEBUG_DUMP
SQInstructionDesc instruction_desc[] = {
	{_LC("_OP_LINE")},
	{_LC("_OP_LOAD")},
	{_LC("_OP_LOADINT")},
	{_LC("_OP_LOADFLOAT")},
	{_LC("_OP_DLOAD")},
	{_LC("_OP_TAILCALL")},
	{_LC("_OP_CALL")},
	{_LC("_OP_PREPCALL")},
	{_LC("_OP_PREPCALLK")},
	{_LC("_OP_GETK")},
	{_LC("_OP_MOVE")},
	{_LC("_OP_NEWSLOT")},
	{_LC("_OP_DELETE")},
	{_LC("_OP_SET")},
	{_LC("_OP_GET")},
	{_LC("_OP_EQ")},
	{_LC("_OP_NE")},
	{_LC("_OP_ADD")},
	{_LC("_OP_SUB")},
	{_LC("_OP_MUL")},
	{_LC("_OP_DIV")},
	{_LC("_OP_MOD")},
	{_LC("_OP_BITW")},
	{_LC("_OP_RETURN")},
	{_LC("_OP_LOADNULLS")},
	{_LC("_OP_LOADROOT")},
	{_LC("_OP_LOADBOOL")},
	{_LC("_OP_DMOVE")},
	{_LC("_OP_JMP")},
	{_LC("_OP_JCMP")},
	{_LC("_OP_JZ")},
	{_LC("_OP_SETOUTER")},
	{_LC("_OP_GETOUTER")},
	{_LC("_OP_NEWOBJ")},
	{_LC("_OP_APPENDARRAY")},
	{_LC("_OP_COMPARITH")},
	{_LC("_OP_INC")},
	{_LC("_OP_INCL")},
	{_LC("_OP_PINC")},
	{_LC("_OP_PINCL")},
	{_LC("_OP_CMP")},
	{_LC("_OP_EXISTS")},
	{_LC("_OP_INSTANCEOF")},
	{_LC("_OP_AND")},
	{_LC("_OP_OR")},
	{_LC("_OP_NEG")},
	{_LC("_OP_NOT")},
	{_LC("_OP_BWNOT")},
	{_LC("_OP_CLOSURE")},
	{_LC("_OP_YIELD")},
	{_LC("_OP_RESUME")},
	{_LC("_OP_FOREACH")},
	{_LC("_OP_POSTFOREACH")},
	{_LC("_OP_CLONE")},
	{_LC("_OP_TYPEOF")},
	{_LC("_OP_PUSHTRAP")},
	{_LC("_OP_POPTRAP")},
	{_LC("_OP_THROW")},
	{_LC("_OP_NEWSLOTA")},
	{_LC("_OP_GETBASE")},
	{_LC("_OP_CLOSE")},
};
#endif

void DumpLiteral(SQObjectPtr& o) {
	switch (type(o)) {
		case OT_STRING:
			scprintf(_LC("\"%s\""), _stringval(o));
			break;
		case OT_FLOAT:
			scprintf(_LC("{%f}"), _float(o));
			break;
		case OT_INTEGER:
			scprintf(_LC("{") _PRINT_INT_FMT _LC("}"), _integer(o));
			break;
		case OT_BOOL:
			scprintf(_LC("%s"), _integer(o) ? _LC("true") : _LC("false"));
			break;
		default:
			scprintf(_LC("(%s %p)"), GetTypeName(o), (void *)_rawval(o));
			break;
	}
}

FunctionState::FunctionState(SQSharedState *ss, FunctionState *parent, CompilerErrorFunc efunc, void *ed) {
	_nliterals = 0;
	_literals = SQTable::Create(ss, 0);
	_strings =  SQTable::Create(ss, 0);
	_sharedstate = ss;
	_lastline = 0;
	_optimization = true;
	_parent = parent;
	_stacksize = 0;
	_traps = 0;
	_returnexp = 0;
	_varparams = false;
	_errfunc = efunc;
	_errtarget = ed;
	_bgenerator = false;
	_outers = 0;
	_ss = ss;
}

void FunctionState::Error(const LVChar *err) {
	_errfunc(_errtarget, err);
}

#ifdef _DEBUG_DUMP
void FunctionState::Dump(FunctionPrototype *func) {
	LVUnsignedInteger n = 0, i;
	LVInteger si;

	scprintf(_LC("--------------------------------------------------------------------\n"));
	scprintf(_LC("function: %s\n"), type(func->_name) == OT_STRING ? _stringval(func->_name) : _LC("unknown"));
	scprintf(_LC("stack size: " _PRINT_INT_FMT "\n"), func->_stacksize);
	if (_varparams)
		scprintf(_LC("varparams: true\n"));

	scprintf(_LC("literals:\n"));
	SQObjectPtr refidx, key, val;
	LVInteger idx;
	SQObjectPtrVec templiterals;
	templiterals.resize(_nliterals);
	while ((idx = _table(_literals)->Next(false, refidx, key, val)) != -1) {
		refidx = idx;
		templiterals[_integer(val)] = key;
	}
	for (i = 0; i < templiterals.size(); i++) {
		scprintf(_LC("\t[" _PRINT_INT_FMT "] "), n);
		DumpLiteral(templiterals[i]);
		scprintf(_LC("\n"));
		n++;
	}
	scprintf(_LC("parameters:\n"));
	n = 0;
	for (i = 0; i < _parameters.size(); i++) {
		scprintf(_LC("\t[" _PRINT_INT_FMT "] "), n);
		DumpLiteral(_parameters[i]);
		scprintf(_LC("\n"));
		n++;
	}
	scprintf(_LC("variables:\n"));
	for (si = 0; si < func->_nlocalvarinfos; si++) {
		SQLocalVarInfo lvi = func->_localvarinfos[si];
		scprintf(_LC("\t[" _PRINT_INT_FMT "] %s \t" _PRINT_INT_FMT " -> " _PRINT_INT_FMT "\n"), lvi._pos, _stringval(lvi._name), lvi._start_op, lvi._end_op);
		n++;
	}
	scprintf(_LC("line info:\n"));
	for (i = 0; i < _lineinfos.size(); i++) {
		SQLineInfo li = _lineinfos[i];
		scprintf(_LC("\top [" _PRINT_INT_FMT "] line [" _PRINT_INT_FMT "] \n"), li._op, li._line);
		n++;
	}
	scprintf(_LC("instruction set:\n"));
	n = 0;
	for (i = 0; i < _instructions.size(); i++) {
		SQInstruction& inst = _instructions[i];
		if (inst.op == _OP_LOAD || inst.op == _OP_DLOAD || inst.op == _OP_PREPCALLK || inst.op == _OP_GETK ) {

			LVInteger lidx = inst._arg1;
			scprintf(_LC("\t[" _PRINT_INT_FMT3 "] %15s %d "), n, instruction_desc[inst.op].name, inst._arg0);
			if (lidx >= 0xFFFFFFFF)
				scprintf(_LC("null"));
			else {
				LVInteger refidx;
				SQObjectPtr val, key, refo;
				while (((refidx = _table(_literals)->Next(false, refo, key, val)) != -1) && (_integer(val) != lidx)) {
					refo = refidx;
				}
				DumpLiteral(key);
			}
			if (inst.op != _OP_DLOAD) {
				scprintf(_LC(" %d %d \n"), inst._arg2, inst._arg3);
			} else {
				scprintf(_LC(" %d "), inst._arg2);
				lidx = inst._arg3;
				if (lidx >= 0xFFFFFFFF)
					scprintf(_LC("null"));
				else {
					LVInteger refidx;
					SQObjectPtr val, key, refo;
					while (((refidx = _table(_literals)->Next(false, refo, key, val)) != -1) && (_integer(val) != lidx)) {
						refo = refidx;
					}
					DumpLiteral(key);
					scprintf(_LC("\n"));
				}
			}
		} else if (inst.op == _OP_LOADFLOAT) {
			scprintf(_LC("\t[" _PRINT_INT_FMT3 "] %15s %d %f %d %d\n"), n, instruction_desc[inst.op].name, inst._arg0, *((LVFloat *)&inst._arg1), inst._arg2, inst._arg3);
		}
		/*  else if(inst.op==_OP_ARITH){
				scprintf(_LC("[%03d] %15s %d %d %d %c\n"),n,instruction_desc[inst.op].name,inst._arg0,inst._arg1,inst._arg2,inst._arg3);
			}*/
		else {
			scprintf(_LC("\t[" _PRINT_INT_FMT3 "] %15s %d %d %d %d\n"), n, instruction_desc[inst.op].name, inst._arg0, inst._arg1, inst._arg2, inst._arg3);
		}
		n++;
	}
	scprintf(_LC("--------------------------------------------------------------------\n"));
}
#endif

LVInteger FunctionState::GetNumericConstant(const LVInteger cons) {
	return GetConstant(SQObjectPtr(cons));
}

LVInteger FunctionState::GetNumericConstant(const LVFloat cons) {
	return GetConstant(SQObjectPtr(cons));
}

LVInteger FunctionState::GetConstant(const SQObject& cons) {
	SQObjectPtr val;
	if (!_table(_literals)->Get(cons, val)) {
		val = _nliterals;
		_table(_literals)->NewSlot(cons, val);
		_nliterals++;
		if (_nliterals > MAX_LITERALS) {
			val.Null();
			Error(_LC("internal compiler error: too many literals"));
		}
	}
	return _integer(val);
}

void FunctionState::SetIntructionParams(LVInteger pos, LVInteger arg0, LVInteger arg1, LVInteger arg2, LVInteger arg3) {
	_instructions[pos]._arg0 = (unsigned char) * ((LVUnsignedInteger *)&arg0);
	_instructions[pos]._arg1 = (LVInt32) * ((LVUnsignedInteger *)&arg1);
	_instructions[pos]._arg2 = (unsigned char) * ((LVUnsignedInteger *)&arg2);
	_instructions[pos]._arg3 = (unsigned char) * ((LVUnsignedInteger *)&arg3);
}

void FunctionState::SetIntructionParam(LVInteger pos, LVInteger arg, LVInteger val) {
	switch (arg) {
		case 0:
			_instructions[pos]._arg0 = (unsigned char) * ((LVUnsignedInteger *)&val);
			break;
		case 1:
		case 4:
			_instructions[pos]._arg1 = (LVInt32) * ((LVUnsignedInteger *)&val);
			break;
		case 2:
			_instructions[pos]._arg2 = (unsigned char) * ((LVUnsignedInteger *)&val);
			break;
		case 3:
			_instructions[pos]._arg3 = (unsigned char) * ((LVUnsignedInteger *)&val);
			break;
	};
}

LVInteger FunctionState::AllocStackPos() {
	LVInteger npos = _vlocals.size();
	_vlocals.push_back(SQLocalVarInfo());
	if (_vlocals.size() > ((LVUnsignedInteger)_stacksize)) {
		if (_stacksize > MAX_FUNC_STACKSIZE) Error(_LC("internal compiler error: too many locals"));
		_stacksize = _vlocals.size();
	}
	return npos;
}

LVInteger FunctionState::PushTarget(LVInteger n) {
	if (n != -1) {
		_targetstack.push_back(n);
		return n;
	}
	n = AllocStackPos();
	_targetstack.push_back(n);
	return n;
}

LVInteger FunctionState::GetUpTarget(LVInteger n) {
	return _targetstack[((_targetstack.size() - 1) - n)];
}

LVInteger FunctionState::TopTarget() {
	return _targetstack.back();
}

LVInteger FunctionState::PopTarget() {
	LVUnsignedInteger npos = _targetstack.back();
	assert(npos < _vlocals.size());
	SQLocalVarInfo& t = _vlocals[npos];
	if (type(t._name) == OT_NULL) {
		_vlocals.pop_back();
	}
	_targetstack.pop_back();
	return npos;
}

LVInteger FunctionState::GetStackSize() {
	return _vlocals.size();
}

LVInteger FunctionState::CountOuters(LVInteger stacksize) {
	LVInteger outers = 0;
	LVInteger k = _vlocals.size() - 1;
	while (k >= stacksize) {
		SQLocalVarInfo& lvi = _vlocals[k];
		k--;
		if (lvi._end_op == UINT_MINUS_ONE) { //this means is an outer
			outers++;
		}
	}
	return outers;
}

void FunctionState::SetStackSize(LVInteger n) {
	LVInteger size = _vlocals.size();
	while (size > n) {
		size--;
		SQLocalVarInfo lvi = _vlocals.back();
		if (type(lvi._name) != OT_NULL) {
			if (lvi._end_op == UINT_MINUS_ONE) { //this means is an outer
				_outers--;
			}
			lvi._end_op = GetCurrentPos();
			_localvarinfos.push_back(lvi);
		}
		_vlocals.pop_back();
	}
}

bool FunctionState::IsConstant(const SQObject& name, SQObject& e) {
	SQObjectPtr val;
	if (_table(_sharedstate->_consts)->Get(name, val)) {
		e = val;
		return true;
	}
	return false;
}

bool FunctionState::IsLocal(LVUnsignedInteger stkpos) {
	if (stkpos >= _vlocals.size())return false;
	else if (type(_vlocals[stkpos]._name) != OT_NULL)return true;
	return false;
}

LVInteger FunctionState::PushLocalVariable(const SQObject& name) {
	LVInteger pos = _vlocals.size();
	SQLocalVarInfo lvi;
	lvi._name = name;
	lvi._start_op = GetCurrentPos() + 1;
	lvi._pos = _vlocals.size();
	_vlocals.push_back(lvi);
	if (_vlocals.size() > ((LVUnsignedInteger)_stacksize))
		_stacksize = _vlocals.size();
	return pos;
}

LVInteger FunctionState::GetLocalVariable(const SQObject& name) {
	LVInteger locals = _vlocals.size();
	while (locals >= 1) {
		SQLocalVarInfo& lvi = _vlocals[locals - 1];
		if (type(lvi._name) == OT_STRING && _string(lvi._name) == _string(name)) {
			return locals - 1;
		}
		locals--;
	}
	return -1;
}

void FunctionState::MarkLocalAsOuter(LVInteger pos) {
	SQLocalVarInfo& lvi = _vlocals[pos];
	lvi._end_op = UINT_MINUS_ONE;
	_outers++;
}

LVInteger FunctionState::GetOuterVariable(const SQObject& name) {
	LVInteger outers = _outervalues.size();
	for (LVInteger i = 0; i < outers; i++) {
		if (_string(_outervalues[i]._name) == _string(name))
			return i;
	}
	LVInteger pos = -1;
	if (_parent) {
		pos = _parent->GetLocalVariable(name);
		if (pos == -1) {
			pos = _parent->GetOuterVariable(name);
			if (pos != -1) {
				_outervalues.push_back(SQOuterVar(name, SQObjectPtr(LVInteger(pos)), otOUTER)); //local
				return _outervalues.size() - 1;
			}
		} else {
			_parent->MarkLocalAsOuter(pos);
			_outervalues.push_back(SQOuterVar(name, SQObjectPtr(LVInteger(pos)), otLOCAL)); //local
			return _outervalues.size() - 1;


		}
	}
	return -1;
}

void FunctionState::AddParameter(const SQObject& name) {
	PushLocalVariable(name);
	_parameters.push_back(name);
}

void FunctionState::AddLineInfos(LVInteger line, bool lineop, bool force) {
	if (_lastline != line || force) {
		SQLineInfo li;
		li._line = line;
		li._op = (GetCurrentPos() + 1);
		if (lineop)AddInstruction(_OP_LINE, 0, line);
		if (_lastline != line) {
			_lineinfos.push_back(li);
		}
		_lastline = line;
	}
}

void FunctionState::DiscardTarget() {
	LVInteger discardedtarget = PopTarget();
	LVInteger size = _instructions.size();
	if (size > 0 && _optimization) {
		SQInstruction& pi = _instructions[size - 1]; //previous instruction
		switch (pi.op) {
			case _OP_SET:
			case _OP_NEWSLOT:
			case _OP_SETOUTER:
			case _OP_CALL:
				if (pi._arg0 == discardedtarget) {
					pi._arg0 = 0xFF;
				}
		}
	}
}

void FunctionState::AddInstruction(SQInstruction& i) {
	LVInteger size = _instructions.size();
	if (size > 0 && _optimization) { //simple optimizer
		SQInstruction& pi = _instructions[size - 1]; //previous instruction
		switch (i.op) {
			case _OP_JZ:
				if ( pi.op == _OP_CMP && pi._arg1 < 0xFF) {
					pi.op = _OP_JCMP;
					pi._arg0 = (unsigned char)pi._arg1;
					pi._arg1 = i._arg1;
					return;
				}
				break;
			case _OP_SET:
			case _OP_NEWSLOT:
				if (i._arg0 == i._arg3) {
					i._arg0 = 0xFF;
				}
				break;
			case _OP_SETOUTER:
				if (i._arg0 == i._arg2) {
					i._arg0 = 0xFF;
				}
				break;
			case _OP_RETURN:
				if ( _parent && i._arg0 != MAX_FUNC_STACKSIZE && pi.op == _OP_CALL && _returnexp < size - 1) {
					pi.op = _OP_TAILCALL;
				} else if (pi.op == _OP_CLOSE) {
					pi = i;
					return;
				}
				break;
			case _OP_GET:
				if ( pi.op == _OP_LOAD && pi._arg0 == i._arg2 && (!IsLocal(pi._arg0))) {
					pi._arg1 = pi._arg1;
					pi._arg2 = (unsigned char)i._arg1;
					pi.op = _OP_GETK;
					pi._arg0 = i._arg0;

					return;
				}
				break;
			case _OP_PREPCALL:
				if ( pi.op == _OP_LOAD  && pi._arg0 == i._arg1 && (!IsLocal(pi._arg0))) {
					pi.op = _OP_PREPCALLK;
					pi._arg0 = i._arg0;
					pi._arg1 = pi._arg1;
					pi._arg2 = i._arg2;
					pi._arg3 = i._arg3;
					return;
				}
				break;
			case _OP_APPENDARRAY: {
				LVInteger aat = -1;
				switch (pi.op) {
					case _OP_LOAD:
						aat = AAT_LITERAL;
						break;
					case _OP_LOADINT:
						aat = AAT_INT;
						break;
					case _OP_LOADBOOL:
						aat = AAT_BOOL;
						break;
					case _OP_LOADFLOAT:
						aat = AAT_FLOAT;
						break;
					default:
						break;
				}
				if (aat != -1 && pi._arg0 == i._arg1 && (!IsLocal(pi._arg0))) {
					pi.op = _OP_APPENDARRAY;
					pi._arg0 = i._arg0;
					pi._arg1 = pi._arg1;
					pi._arg2 = (unsigned char)aat;
					pi._arg3 = MAX_FUNC_STACKSIZE;
					return;
				}
			}
			break;
			case _OP_MOVE:
				switch (pi.op) {
					case _OP_GET:
					case _OP_ADD:
					case _OP_SUB:
					case _OP_MUL:
					case _OP_DIV:
					case _OP_MOD:
					case _OP_BITW:
					case _OP_LOADINT:
					case _OP_LOADFLOAT:
					case _OP_LOADBOOL:
					case _OP_LOAD:

						if (pi._arg0 == i._arg1) {
							pi._arg0 = i._arg0;
							_optimization = false;
							//_result_elimination = false;
							return;
						}
				}

				if (pi.op == _OP_MOVE) {
					pi.op = _OP_DMOVE;
					pi._arg2 = i._arg0;
					pi._arg3 = (unsigned char)i._arg1;
					return;
				}
				break;
			case _OP_LOAD:
				if (pi.op == _OP_LOAD && i._arg1 < 256) {
					pi.op = _OP_DLOAD;
					pi._arg2 = i._arg0;
					pi._arg3 = (unsigned char)i._arg1;
					return;
				}
				break;
			case _OP_EQ:
			case _OP_NE:
				if (pi.op == _OP_LOAD && pi._arg0 == i._arg1 && (!IsLocal(pi._arg0) )) {
					pi.op = i.op;
					pi._arg0 = i._arg0;
					pi._arg1 = pi._arg1;
					pi._arg2 = i._arg2;
					pi._arg3 = MAX_FUNC_STACKSIZE;
					return;
				}
				break;
			case _OP_LOADNULLS:
				if ((pi.op == _OP_LOADNULLS && pi._arg0 + pi._arg1 == i._arg0)) {

					pi._arg1 = pi._arg1 + 1;
					pi.op = _OP_LOADNULLS;
					return;
				}
				break;
			case _OP_LINE:
				if (pi.op == _OP_LINE) {
					_instructions.pop_back();
					_lineinfos.pop_back();
				}
				break;
		}
	}
	_optimization = true;
	_instructions.push_back(i);
}

SQObject FunctionState::CreateString(const LVChar *s, LVInteger len) {
	SQObjectPtr ns(SQString::Create(_sharedstate, s, len));
	_table(_strings)->NewSlot(ns, (LVInteger)1);
	return ns;
}

SQObject FunctionState::CreateTable() {
	SQObjectPtr nt(SQTable::Create(_sharedstate, 0));
	_table(_strings)->NewSlot(nt, (LVInteger)1);
	return nt;
}

FunctionPrototype *FunctionState::BuildProto() {
	FunctionPrototype *f = FunctionPrototype::Create(_ss, _instructions.size(),
	                       _nliterals, _parameters.size(), _functions.size(), _outervalues.size(),
	                       _lineinfos.size(), _localvarinfos.size(), _defaultparams.size());

	SQObjectPtr refidx, key, val;
	LVInteger idx;

	f->_stacksize = _stacksize;
	f->_sourcename = _sourcename;
	f->_bgenerator = _bgenerator;
	f->_name = _name;

	while ((idx = _table(_literals)->Next(false, refidx, key, val)) != -1) {
		f->_literals[_integer(val)] = key;
		refidx = idx;
	}

	for (LVUnsignedInteger nf = 0; nf < _functions.size(); nf++) f->_functions[nf] = _functions[nf];
	for (LVUnsignedInteger np = 0; np < _parameters.size(); np++) f->_parameters[np] = _parameters[np];
	for (LVUnsignedInteger no = 0; no < _outervalues.size(); no++) f->_outervalues[no] = _outervalues[no];
	for (LVUnsignedInteger nl = 0; nl < _localvarinfos.size(); nl++) f->_localvarinfos[nl] = _localvarinfos[nl];
	for (LVUnsignedInteger ni = 0; ni < _lineinfos.size(); ni++) f->_lineinfos[ni] = _lineinfos[ni];
	for (LVUnsignedInteger nd = 0; nd < _defaultparams.size(); nd++) f->_defaultparams[nd] = _defaultparams[nd];

	memcpy(f->_instructions, &_instructions[0], _instructions.size()*sizeof(SQInstruction));

	f->_varparams = _varparams;

	return f;
}

FunctionState *FunctionState::PushChildState(SQSharedState *ss) {
	FunctionState *child = (FunctionState *)lv_malloc(sizeof(FunctionState));
	new (child) FunctionState(ss, this, _errfunc, _errtarget);
	_childstates.push_back(child);
	return child;
}

void FunctionState::PopChildState() {
	FunctionState *child = _childstates.back();
	sq_delete(child, FunctionState);
	_childstates.pop_back();
}

FunctionState::~FunctionState() {
	while (_childstates.size() > 0) {
		PopChildState();
	}
}

#endif
