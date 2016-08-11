#ifndef _USERDATA_H_
#define _USERDATA_H_

struct SQUserData : SQDelegable {
	SQUserData(SQSharedState *ss) {
		_delegate = 0;
		_hook = NULL;
		INIT_CHAIN();
		ADD_TO_CHAIN(&_ss(this)->_gc_chain, this);
	}

	~SQUserData() {
		REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain, this);
		SetDelegate(NULL);
	}

	static SQUserData *Create(SQSharedState *ss, SQInteger size) {
		SQUserData *ud = (SQUserData *)LV_MALLOC(sq_aligning(sizeof(SQUserData)) + size);
		new (ud) SQUserData(ss);
		ud->_size = size;
		ud->_typetag = 0;
		return ud;
	}

#ifndef NO_GARBAGE_COLLECTOR
	void Mark(SQCollectable **chain);
	void Finalize() {
		SetDelegate(NULL);
	}
	SQObjectType GetType() {
		return OT_USERDATA;
	}
#endif

	void Release() {
		if (_hook) _hook((LVUserPointer)sq_aligning(this + 1), _size);
		SQInteger tsize = _size;
		this->~SQUserData();
		LV_FREE(this, sq_aligning(sizeof(SQUserData)) + tsize);
	}

	SQInteger _size;
	SQRELEASEHOOK _hook;
	LVUserPointer _typetag;
};

#endif // _USERDATA_H_
