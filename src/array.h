#ifndef _ARRAY_H_
#define _ARRAY_H_

struct SQArray : public CHAINABLE_OBJ {
  private:
	SQArray(SQSharedState *ss, LVInteger nsize) {
		_values.resize(nsize);
		INIT_CHAIN();
		ADD_TO_CHAIN(&_ss(this)->_gc_chain, this);
	}

	~SQArray() {
		REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain, this);
	}

  public:
	static SQArray *Create(SQSharedState *ss, LVInteger nInitialSize) {
		SQArray *newarray = (SQArray *)LV_MALLOC(sizeof(SQArray));
		new (newarray) SQArray(ss, nInitialSize);
		return newarray;
	}

#ifndef NO_GARBAGE_COLLECTOR
	void Mark(SQCollectable **chain);
	SQObjectType GetType() {
		return OT_ARRAY;
	}
#endif

	void Finalize() {
		_values.resize(0);
	}

	bool Get(const LVInteger nidx, SQObjectPtr& val) {
		if (nidx >= 0 && nidx < (LVInteger)_values.size()) {
			SQObjectPtr& o = _values[nidx];
			val = _realval(o);
			return true;
		} else return false;
	}

	bool Set(const LVInteger nidx, const SQObjectPtr& val) {
		if (nidx >= 0 && nidx < (LVInteger)_values.size()) {
			_values[nidx] = val;
			return true;
		} else return false;
	}

	LVInteger Next(const SQObjectPtr& refpos, SQObjectPtr& outkey, SQObjectPtr& outval) {
		LVUnsignedInteger idx = TranslateIndex(refpos);
		while (idx < _values.size()) {
			//first found
			outkey = (LVInteger)idx;
			SQObjectPtr& o = _values[idx];
			outval = _realval(o);
			//return idx for the next iteration
			return ++idx;
		}
		//nothing to iterate anymore
		return -1;
	}

	SQArray *Clone() {
		SQArray *anew = Create(_opt_ss(this), 0);
		anew->_values.copy(_values);
		return anew;
	}

	LVInteger Size() const {
		return _values.size();
	}

	void Resize(LVInteger size) {
		SQObjectPtr _null;
		Resize(size, _null);
	}

	void Resize(LVInteger size, SQObjectPtr& fill) {
		_values.resize(size, fill);
		ShrinkIfNeeded();
	}

	void Reserve(LVInteger size) {
		_values.reserve(size);
	}

	void Append(const SQObject& o) {
		_values.push_back(o);
	}

	void Extend(const SQArray *a);

	SQObjectPtr& Top() {
		return _values.top();
	}

	void Pop() {
		_values.pop_back();
		ShrinkIfNeeded();
	}

	bool Insert(LVInteger idx, const SQObject& val) {
		if (idx < 0 || idx > (LVInteger)_values.size())
			return false;
		_values.insert(idx, val);
		return true;
	}

	void ShrinkIfNeeded() {
		if (_values.size() <= _values.capacity() >> 2) //shrink the array
			_values.shrinktofit();
	}

	bool Remove(LVInteger idx) {
		if (idx < 0 || idx >= (LVInteger)_values.size())
			return false;
		_values.remove(idx);
		ShrinkIfNeeded();
		return true;
	}

	void Release() {
		sq_delete(this, SQArray);
	}

	SQObjectPtrVec _values;
};
#endif // _ARRAY_H_
