#ifndef _CLASS_H_
#define _CLASS_H_

struct LVInstance;

struct LVClassMember {
	LVObjectPtr val;
	LVObjectPtr attrs;
	void Null() {
		val.Null();
		attrs.Null();
	}
};

typedef lvvector<LVClassMember> LVClassMemberVec;

#define MEMBER_TYPE_METHOD 0x01000000
#define MEMBER_TYPE_FIELD 0x02000000

#define _ismethod(o) (_integer(o)&MEMBER_TYPE_METHOD)
#define _isfield(o) (_integer(o)&MEMBER_TYPE_FIELD)
#define _make_method_idx(i) ((LVInteger)(MEMBER_TYPE_METHOD|i))
#define _make_field_idx(i) ((LVInteger)(MEMBER_TYPE_FIELD|i))
#define _member_type(o) (_integer(o)&0xFF000000)
#define _member_idx(o) (_integer(o)&0x00FFFFFF)

struct LVClass : public CHAINABLE_OBJ {
	LVClass(LVSharedState *ss, LVClass *base);

  public:
	static LVClass *Create(LVSharedState *ss, LVClass *base) {
		LVClass *newclass = (LVClass *)LV_MALLOC(sizeof(LVClass));
		new (newclass) LVClass(ss, base);
		return newclass;
	}

	~LVClass();

	bool NewSlot(LVSharedState *ss, const LVObjectPtr& key, const LVObjectPtr& val, bool bstatic);

	bool Get(const LVObjectPtr& key, LVObjectPtr& val) {
		if (_members->Get(key, val)) {
			if (_isfield(val)) {
				LVObjectPtr& o = _defaultvalues[_member_idx(val)].val;
				val = _realval(o);
			} else {
				val = _methods[_member_idx(val)].val;
			}
			return true;
		}
		return false;
	}

	bool GetConstructor(LVObjectPtr& ctor) {
		if (_constructoridx != -1) {
			ctor = _methods[_constructoridx].val;
			return true;
		}
		return false;
	}

	bool SetAttributes(const LVObjectPtr& key, const LVObjectPtr& val);
	bool GetAttributes(const LVObjectPtr& key, LVObjectPtr& outval);

	void Lock() {
		_locked = true;
		if (_base) _base->Lock();
	}

	void Release() {
		if (_hook) {
			_hook(_typetag, 0);
		}
		lv_delete(this, LVClass);
	}
	void Finalize();

#ifndef NO_GARBAGE_COLLECTOR
	void Mark(LVCollectable ** );

	LVObjectType GetType() {
		return OT_CLASS;
	}
#endif

	LVInteger Next(const LVObjectPtr& refpos, LVObjectPtr& outkey, LVObjectPtr& outval);
	LVInstance *CreateInstance();
	LVTable *_members;
	LVClass *_base;
	LVClassMemberVec _defaultvalues;
	LVClassMemberVec _methods;
	LVObjectPtr _metamethods[MT_LAST];
	LVObjectPtr _attributes;
	LVUserPointer _typetag;
	LVRELEASEHOOK _hook;
	bool _locked;
	LVInteger _constructoridx;
	LVInteger _udsize;
};

#define calcinstancesize(_theclass_) \
    (_theclass_->_udsize + LV_ALIGN(sizeof(LVInstance) +  (sizeof(LVObjectPtr)*(_theclass_->_defaultvalues.size()>0?_theclass_->_defaultvalues.size()-1:0))))

struct LVInstance : public LVDelegable {
	void Init(LVSharedState *ss);
	LVInstance(LVSharedState *ss, LVClass *c, LVInteger memsize);
	LVInstance(LVSharedState *ss, LVInstance *c, LVInteger memsize);

  public:
	static LVInstance *Create(LVSharedState *ss, LVClass *theclass) {
		LVInteger size = calcinstancesize(theclass);
		LVInstance *newinst = (LVInstance *)LV_MALLOC(size);
		new (newinst) LVInstance(ss, theclass, size);
		if (theclass->_udsize) {
			newinst->_userpointer = ((unsigned char *)newinst) + (size - theclass->_udsize);
		}
		return newinst;
	}

	LVInstance *Clone(LVSharedState *ss) {
		LVInteger size = calcinstancesize(_class);
		LVInstance *newinst = (LVInstance *)LV_MALLOC(size);
		new (newinst) LVInstance(ss, this, size);
		if (_class->_udsize) {
			newinst->_userpointer = ((unsigned char *)newinst) + (size - _class->_udsize);
		}
		return newinst;
	}

	~LVInstance();

	bool Get(const LVObjectPtr& key, LVObjectPtr& val)  {
		if (_class->_members->Get(key, val)) {
			if (_isfield(val)) {
				LVObjectPtr& o = _values[_member_idx(val)];
				val = _realval(o);
			} else {
				val = _class->_methods[_member_idx(val)].val;
			}
			return true;
		}
		return false;
	}

	bool Set(const LVObjectPtr& key, const LVObjectPtr& val) {
		LVObjectPtr idx;
		if (_class->_members->Get(key, idx) && _isfield(idx)) {
			_values[_member_idx(idx)] = val;
			return true;
		}
		return false;
	}

	void Release() {
		_uiRef++;
		if (_hook) {
			_hook(_userpointer, 0);
		}
		_uiRef--;
		if (_uiRef > 0) return;
		LVInteger size = _memsize;
		this->~LVInstance();
		LV_FREE(this, size);
	}

	void Finalize();

#ifndef NO_GARBAGE_COLLECTOR
	void Mark(LVCollectable ** );

	LVObjectType GetType() {
		return OT_INSTANCE;
	}
#endif

	bool InstanceOf(LVClass *trg);
	bool GetMetaMethod(LVVM *v, LVMetaMethod mm, LVObjectPtr& res);

	LVClass *_class;
	LVUserPointer _userpointer;
	LVRELEASEHOOK _hook;
	LVInteger _memsize;
	LVObjectPtr _values[1];
};

#endif // _CLASS_H_
