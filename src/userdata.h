#ifndef _USERDATA_H_
#define _USERDATA_H_

struct LVUserData : LVDelegable {
	LVUserData(LVSharedState *ss) {
		_delegate = 0;
		_hook = NULL;
		INIT_CHAIN();
		ADD_TO_CHAIN(&_ss(this)->_gc_chain, this);
	}

	~LVUserData() {
		REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain, this);
		SetDelegate(NULL);
	}

	static LVUserData *Create(LVSharedState *ss, LVInteger size) {
		LVUserData *ud = (LVUserData *)LV_MALLOC(LV_ALIGN(sizeof(LVUserData)) + size);
		new (ud) LVUserData(ss);
		ud->_size = size;
		ud->_typetag = 0;
		return ud;
	}

#ifndef NO_GARBAGE_COLLECTOR
	void Mark(LVCollectable **chain);
	void Finalize() {
		SetDelegate(NULL);
	}
	LVObjectType GetType() {
		return OT_USERDATA;
	}
#endif

	void Release() {
		if (_hook)
			_hook((LVUserPointer)LV_ALIGN(this + 1), _size);
		LVInteger tsize = _size;
		this->~LVUserData();
		LV_FREE(this, LV_ALIGN(sizeof(LVUserData)) + tsize);
	}

	LVInteger _size;
	LVRELEASEHOOK _hook;
	LVUserPointer _typetag;
};

#endif // _USERDATA_H_
