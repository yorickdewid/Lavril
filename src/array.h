#ifndef _ARRAY_H_
#define _ARRAY_H_

struct LVArray : public CHAINABLE_OBJ {
  private:
	LVArray(LVSharedState *ss, LVInteger nsize) {
		_values.resize(nsize);
		INIT_CHAIN();
		ADD_TO_CHAIN(&_ss(this)->_gc_chain, this);
	}

	~LVArray() {
		REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain, this);
	}

  public:
	static LVArray *Create(LVSharedState *ss, LVInteger nInitialSize) {
		LVArray *newarray = (LVArray *)LV_MALLOC(sizeof(LVArray));
		new (newarray) LVArray(ss, nInitialSize);
		return newarray;
	}

#ifndef NO_GARBAGE_COLLECTOR
	void Mark(LVCollectable **chain);
	LVObjectType GetType() {
		return OT_ARRAY;
	}
#endif

	void Finalize() {
		_values.resize(0);
	}

	bool Get(const LVInteger nidx, LVObjectPtr& val) {
		if (nidx >= 0 && nidx < (LVInteger)_values.size()) {
			LVObjectPtr& o = _values[nidx];
			val = _realval(o);
			return true;
		} else return false;
	}

	bool Set(const LVInteger nidx, const LVObjectPtr& val) {
		if (nidx >= 0 && nidx < (LVInteger)_values.size()) {
			_values[nidx] = val;
			return true;
		} else return false;
	}

	LVInteger Next(const LVObjectPtr& refpos, LVObjectPtr& outkey, LVObjectPtr& outval) {
		LVUnsignedInteger idx = TranslateIndex(refpos);
		while (idx < _values.size()) {
			//first found
			outkey = (LVInteger)idx;
			LVObjectPtr& o = _values[idx];
			outval = _realval(o);
			//return idx for the next iteration
			return ++idx;
		}
		//nothing to iterate anymore
		return -1;
	}

	LVArray *Clone() {
		LVArray *anew = Create(_opt_ss(this), 0);
		anew->_values.copy(_values);
		return anew;
	}

	LVInteger Size() const {
		return _values.size();
	}

	void Resize(LVInteger size) {
		LVObjectPtr _null;
		Resize(size, _null);
	}

	void Resize(LVInteger size, LVObjectPtr& fill) {
		_values.resize(size, fill);
		ShrinkIfNeeded();
	}

	void Reserve(LVInteger size) {
		_values.reserve(size);
	}

	void Append(const LVObject& o) {
		_values.push_back(o);
	}

	void Extend(const LVArray *a);

	LVObjectPtr& Top() {
		return _values.top();
	}

	void Pop() {
		_values.pop_back();
		ShrinkIfNeeded();
	}

	bool Insert(LVInteger idx, const LVObject& val) {
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
		lv_delete(this, LVArray);
	}

	LVObjectPtrVec _values;
};
#endif // _ARRAY_H_
