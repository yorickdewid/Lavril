#include "pcheader.h"
#include "vm.h"
#include "table.h"
#include "class.h"
#include "funcproto.h"
#include "closure.h"

LVClass::LVClass(LVSharedState *ss, LVClass *base) {
	_base = base;
	_typetag = 0;
	_hook = NULL;
	_udsize = 0;
	_abstract = false;
	_locked = false;
	_constructoridx = -1;
	if (_base) {
		_constructoridx = _base->_constructoridx;
		_udsize = _base->_udsize;
		_defaultvalues.copy(base->_defaultvalues);
		_methods.copy(base->_methods);
		_COPY_VECTOR(_metamethods, base->_metamethods, MT_LAST);
		__ObjAddRef(_base);
	}
	_members = base ? base->_members->Clone() : LVTable::Create(ss, 0);
	__ObjAddRef(_members);

	INIT_CHAIN();
	ADD_TO_CHAIN(&_sharedstate->_gc_chain, this);
}

void LVClass::Finalize() {
	_attributes.Null();
	_NULL_OBJECT_VECTOR(_defaultvalues, _defaultvalues.size());
	_methods.resize(0);
	_NULL_OBJECT_VECTOR(_metamethods, MT_LAST);
	__ObjRelease(_members);
	if (_base) {
		__ObjRelease(_base);
	}
}

LVClass::~LVClass() {
	REMOVE_FROM_CHAIN(&_sharedstate->_gc_chain, this);
	Finalize();
}

bool LVClass::NewSlot(LVSharedState *ss, const LVObjectPtr& key, const LVObjectPtr& val, bool bstatic) {
	LVObjectPtr temp;
	bool belongs_to_static_table = type(val) == OT_CLOSURE || type(val) == OT_NATIVECLOSURE || bstatic;
	if (_locked && !belongs_to_static_table)
		return false; //the class already has an instance so cannot be modified
	if (_members->Get(key, temp) && _isfield(temp)) { //overrides the default value
		_defaultvalues[_member_idx(temp)].val = val;
		return true;
	}
	if (belongs_to_static_table) {
		LVInteger mmidx;
		if ((type(val) == OT_CLOSURE || type(val) == OT_NATIVECLOSURE) &&
		        (mmidx = ss->GetMetaMethodIdxByName(key)) != -1) {
			_metamethods[mmidx] = val;
		} else {
			LVObjectPtr theval = val;
			if (_base && type(val) == OT_CLOSURE) {
				theval = _closure(val)->Clone();
				_closure(theval)->_base = _base;
				__ObjAddRef(_base); //ref for the closure
			}
			if (type(temp) == OT_NULL) {
				bool isconstructor;
				LVVM::IsEqual(ss->_constructoridx, key, isconstructor);
				if (isconstructor) {
					_constructoridx = (LVInteger)_methods.size();
				}
				LVClassMember m;
				m.val = theval;
				_members->NewSlot(key, LVObjectPtr(_make_method_idx(_methods.size())));
				_methods.push_back(m);
			} else {
				_methods[_member_idx(temp)].val = theval;
			}
		}
		return true;
	}
	LVClassMember m;
	m.val = val;
	_members->NewSlot(key, LVObjectPtr(_make_field_idx(_defaultvalues.size())));
	_defaultvalues.push_back(m);
	return true;
}

LVInstance *LVClass::CreateInstance() {
	if (!_locked) Lock();
	return LVInstance::Create(_opt_ss(this), this);
}

LVInteger LVClass::Next(const LVObjectPtr& refpos, LVObjectPtr& outkey, LVObjectPtr& outval) {
	LVObjectPtr oval;
	LVInteger idx = _members->Next(false, refpos, outkey, oval);
	if (idx != -1) {
		if (_ismethod(oval)) {
			outval = _methods[_member_idx(oval)].val;
		} else {
			LVObjectPtr& o = _defaultvalues[_member_idx(oval)].val;
			outval = _realval(o);
		}
	}
	return idx;
}

bool LVClass::SetAttributes(const LVObjectPtr& key, const LVObjectPtr& val) {
	LVObjectPtr idx;
	if (_members->Get(key, idx)) {
		if (_isfield(idx))
			_defaultvalues[_member_idx(idx)].attrs = val;
		else
			_methods[_member_idx(idx)].attrs = val;
		return true;
	}
	return false;
}

bool LVClass::GetAttributes(const LVObjectPtr& key, LVObjectPtr& outval) {
	LVObjectPtr idx;
	if (_members->Get(key, idx)) {
		outval = (_isfield(idx) ? _defaultvalues[_member_idx(idx)].attrs : _methods[_member_idx(idx)].attrs);
		return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////
void LVInstance::Init(LVSharedState *ss) {
	_userpointer = NULL;
	_hook = NULL;
	__ObjAddRef(_class);
	_delegate = _class->_members;
	INIT_CHAIN();
	ADD_TO_CHAIN(&_sharedstate->_gc_chain, this);
}

LVInstance::LVInstance(LVSharedState *ss, LVClass *c, LVInteger memsize) {
	_memsize = memsize;
	_class = c;
	LVUnsignedInteger nvalues = _class->_defaultvalues.size();
	for (LVUnsignedInteger n = 0; n < nvalues; n++) {
		new (&_values[n]) LVObjectPtr(_class->_defaultvalues[n].val);
	}
	Init(ss);
}

LVInstance::LVInstance(LVSharedState *ss, LVInstance *i, LVInteger memsize) {
	_memsize = memsize;
	_class = i->_class;
	LVUnsignedInteger nvalues = _class->_defaultvalues.size();
	for (LVUnsignedInteger n = 0; n < nvalues; n++) {
		new (&_values[n]) LVObjectPtr(i->_values[n]);
	}
	Init(ss);
}

void LVInstance::Finalize() {
	LVUnsignedInteger nvalues = _class->_defaultvalues.size();
	__ObjRelease(_class);
	_NULL_OBJECT_VECTOR(_values, nvalues);
}

LVInstance::~LVInstance() {
	REMOVE_FROM_CHAIN(&_sharedstate->_gc_chain, this);
	if (_class) {
		Finalize();    //if _class is null it was already finalized by the GC
	}
}

bool LVInstance::GetMetaMethod(LVVM LV_UNUSED_ARG(*v), LVMetaMethod mm, LVObjectPtr& res) {
	if (type(_class->_metamethods[mm]) != OT_NULL) {
		res = _class->_metamethods[mm];
		return true;
	}
	return false;
}

bool LVInstance::InstanceOf(LVClass *trg) {
	LVClass *parent = _class;
	while (parent != NULL) {
		if (parent == trg)
			return true;
		parent = parent->_base;
	}
	return false;
}
