#ifndef _UTILS_H_
#define _UTILS_H_

void *lv_vm_malloc(SQUnsignedInteger size);
void *lv_vm_realloc(void *p, SQUnsignedInteger oldsize, SQUnsignedInteger size);
void lv_vm_free(void *p, SQUnsignedInteger size);

#define sq_new(__ptr,__type) {__ptr=(__type *)lv_vm_malloc(sizeof(__type));new (__ptr) __type;}
#define sq_delete(__ptr,__type) {__ptr->~__type();lv_vm_free(__ptr,sizeof(__type));}

#define LV_MALLOC(__size) lv_vm_malloc((__size));
#define LV_FREE(__ptr,__size) lv_vm_free((__ptr),(__size));
#define LV_REALLOC(__ptr,__oldsize,__size) lv_vm_realloc((__ptr),(__oldsize),(__size));

#define LV_ALIGN(v) (((size_t)(v) + (LV_ALIGNMENT-1)) & (~(LV_ALIGNMENT-1)))

template<typename T>
class sqvector {
  public:
	sqvector(SQUnsignedInteger size = 0) {
		_vals = NULL;
		_size = 0;
		_allocated = 0;

		if (size)
			resize(size);
	}

	sqvector(const sqvector<T>& v) {
		copy(v);
	}

	void copy(const sqvector<T>& v) {
		if (_size) {
			resize(0); //destroys all previous stuff
		}

		if (v._size > _allocated) {
			_realloc(v._size);
		}
		for (SQUnsignedInteger i = 0; i < v._size; i++) {
			new ((void *)&_vals[i]) T(v._vals[i]);
		}
		_size = v._size;
	}

	~sqvector() {
		if (_allocated) {
			for (SQUnsignedInteger i = 0; i < _size; i++)
				_vals[i].~T();
			LV_FREE(_vals, (_allocated * sizeof(T)));
		}
	}

	void reserve(SQUnsignedInteger newsize) {
		_realloc(newsize);
	}

	void resize(SQUnsignedInteger newsize, const T& fill = T()) {
		if (newsize > _allocated)
			_realloc(newsize);
		if (newsize > _size) {
			while (_size < newsize) {
				new ((void *)&_vals[_size]) T(fill);
				_size++;
			}
		} else {
			for (SQUnsignedInteger i = newsize; i < _size; i++) {
				_vals[i].~T();
			}
			_size = newsize;
		}
	}

	void shrinktofit() {
		if (_size > 4) {
			_realloc(_size);
		}
	}

	T& top() const {
		return _vals[_size - 1];
	}

	inline SQUnsignedInteger size() const {
		return _size;
	}

	bool empty() const {
		return (_size <= 0);
	}

	inline T& push_back(const T& val = T()) {
		if (_allocated <= _size)
			_realloc(_size * 2);
		return *(new ((void *)&_vals[_size++]) T(val));
	}

	inline void pop_back() {
		_size--;
		_vals[_size].~T();
	}

	void insert(SQUnsignedInteger idx, const T& val) {
		resize(_size + 1);
		for (SQUnsignedInteger i = _size - 1; i > idx; i--) {
			_vals[i] = _vals[i - 1];
		}
		_vals[idx] = val;
	}

	void remove(SQUnsignedInteger idx) {
		_vals[idx].~T();
		if (idx < (_size - 1)) {
			memmove(&_vals[idx], &_vals[idx + 1], sizeof(T) * (_size - idx - 1));
		}
		_size--;
	}

	SQUnsignedInteger capacity() {
		return _allocated;
	}

	inline T& back() const {
		return _vals[_size - 1];
	}

	inline T& operator[](SQUnsignedInteger pos) const {
		return _vals[pos];
	}

	T *_vals;

  private:
	void _realloc(SQUnsignedInteger newsize) {
		newsize = (newsize > 0) ? newsize : 4;
		_vals = (T *)LV_REALLOC(_vals, _allocated * sizeof(T), newsize * sizeof(T));
		_allocated = newsize;
	}
	SQUnsignedInteger _size;
	SQUnsignedInteger _allocated;
};

template<typename T1, typename T2>
struct lvpair {
	T1 first;
	T2 second;

	lvpair(T1 _first, T1 _second) : first(_first), second(_second) {}
	lvpair() {}
};

#endif // _UTILS_H_
