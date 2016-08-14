#ifndef _CLOSURE_H_
#define _CLOSURE_H_

#define _CALC_CLOSURE_SIZE(func) (sizeof(LVClosure) + (func->_noutervalues*sizeof(LVObjectPtr)) + (func->_ndefaultparams*sizeof(LVObjectPtr)))

struct FunctionPrototype;
struct LVClass;
struct LVClosure : public CHAINABLE_OBJ {
  private:
	LVClosure(LVSharedState *ss, FunctionPrototype *func) {
		_function = func;
		__ObjAddRef(_function);
		_base = NULL;
		INIT_CHAIN();
		ADD_TO_CHAIN(&_ss(this)->_gc_chain, this);
		_env = NULL;
		_root = NULL;
	}

  public:
	static LVClosure *Create(LVSharedState *ss, FunctionPrototype *func, LVWeakRef *root) {
		LVInteger size = _CALC_CLOSURE_SIZE(func);
		LVClosure *nc = (LVClosure *)LV_MALLOC(size);
		new (nc) LVClosure(ss, func);
		nc->_outervalues = (LVObjectPtr *)(nc + 1);
		nc->_defaultparams = &nc->_outervalues[func->_noutervalues];
		nc->_root = root;
		__ObjAddRef(nc->_root);
		_CONSTRUCT_VECTOR(LVObjectPtr, func->_noutervalues, nc->_outervalues);
		_CONSTRUCT_VECTOR(LVObjectPtr, func->_ndefaultparams, nc->_defaultparams);
		return nc;
	}

	void Release() {
		FunctionPrototype *f = _function;
		LVInteger size = _CALC_CLOSURE_SIZE(f);
		_DESTRUCT_VECTOR(LVObjectPtr, f->_noutervalues, _outervalues);
		_DESTRUCT_VECTOR(LVObjectPtr, f->_ndefaultparams, _defaultparams);
		__ObjRelease(_function);
		this->~LVClosure();
		lv_vm_free(this, size);
	}

	void SetRoot(LVWeakRef *r) {
		__ObjRelease(_root);
		_root = r;
		__ObjAddRef(_root);
	}

	LVClosure *Clone() {
		FunctionPrototype *f = _function;
		LVClosure *ret = LVClosure::Create(_opt_ss(this), f, _root);
		ret->_env = _env;
		if (ret->_env) __ObjAddRef(ret->_env);
		_COPY_VECTOR(ret->_outervalues, _outervalues, f->_noutervalues);
		_COPY_VECTOR(ret->_defaultparams, _defaultparams, f->_ndefaultparams);
		return ret;
	}

	~LVClosure();

	bool Save(LVVM *v, LVUserPointer up, LVWRITEFUNC write);
	static bool Load(LVVM *v, LVUserPointer up, LVREADFUNC read, LVObjectPtr& ret);

#ifndef NO_GARBAGE_COLLECTOR
	void Mark(LVCollectable **chain);

	void Finalize() {
		FunctionPrototype *f = _function;
		_NULL_OBJECT_VECTOR(_outervalues, f->_noutervalues);
		_NULL_OBJECT_VECTOR(_defaultparams, f->_ndefaultparams);
	}

	LVObjectType GetType() {
		return OT_CLOSURE;
	}
#endif

	LVWeakRef *_env;
	LVWeakRef *_root;
	LVClass *_base;
	FunctionPrototype *_function;
	LVObjectPtr *_outervalues;
	LVObjectPtr *_defaultparams;
};

struct LVOuter : public CHAINABLE_OBJ {
  private:
	LVOuter(LVSharedState *ss, LVObjectPtr *outer) {
		_valptr = outer;
		_next = NULL;
		INIT_CHAIN();
		ADD_TO_CHAIN(&_ss(this)->_gc_chain, this);
	}

  public:
	static LVOuter *Create(LVSharedState *ss, LVObjectPtr *outer) {
		LVOuter *nc  = (LVOuter *)LV_MALLOC(sizeof(LVOuter));
		new (nc) LVOuter(ss, outer);
		return nc;
	}

	~LVOuter() {
		REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain, this);
	}

	void Release() {
		this->~LVOuter();
		lv_vm_free(this, sizeof(LVOuter));
	}

#ifndef NO_GARBAGE_COLLECTOR
	void Mark(LVCollectable **chain);

	void Finalize() {
		_value.Null();
	}

	LVObjectType GetType() {
		return OT_OUTER;
	}
#endif

	LVObjectPtr *_valptr;  /* pointer to value on stack, or _value below */
	LVInteger    _idx;     /* idx in stack array, for relocation */
	LVObjectPtr  _value;   /* value of outer after stack frame is closed */
	LVOuter     *_next;    /* pointer to next outer when frame is open   */
};

struct LVGenerator : public CHAINABLE_OBJ {
	enum LVGeneratorState {
		eRunning,
		eSuspended,
		eDead
	};

  private:
	LVGenerator(LVSharedState *ss, LVClosure *closure) {
		_closure = closure;
		_state = eRunning;
		_ci._generator = NULL;
		INIT_CHAIN();
		ADD_TO_CHAIN(&_ss(this)->_gc_chain, this);
	}

  public:
	static LVGenerator *Create(LVSharedState *ss, LVClosure *closure) {
		LVGenerator *nc = (LVGenerator *)LV_MALLOC(sizeof(LVGenerator));
		new (nc) LVGenerator(ss, closure);
		return nc;
	}

	~LVGenerator() {
		REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain, this);
	}

	void Kill() {
		_state = eDead;
		_stack.resize(0);
		_closure.Null();
	}

	void Release() {
		lv_delete(this, LVGenerator);
	}

	bool Yield(LVVM *v, LVInteger target);
	bool Resume(LVVM *v, LVObjectPtr& dest);
#ifndef NO_GARBAGE_COLLECTOR
	void Mark(LVCollectable **chain);

	void Finalize() {
		_stack.resize(0);
		_closure.Null();
	}

	LVObjectType GetType() {
		return OT_GENERATOR;
	}
#endif

	LVObjectPtr _closure;
	LVObjectPtrVec _stack;
	LVVM::CallInfo _ci;
	ExceptionsTraps _etraps;
	LVGeneratorState _state;
};

#define _CALC_NATVIVECLOSURE_SIZE(noutervalues) (sizeof(LVNativeClosure) + (noutervalues*sizeof(LVObjectPtr)))

struct LVNativeClosure : public CHAINABLE_OBJ {
  private:
	LVNativeClosure(LVSharedState *ss, LVFUNCTION func) {
		_function = func;
		INIT_CHAIN();
		ADD_TO_CHAIN(&_ss(this)->_gc_chain, this);
		_env = NULL;
	}

  public:
	static LVNativeClosure *Create(LVSharedState *ss, LVFUNCTION func, LVInteger nouters) {
		LVInteger size = _CALC_NATVIVECLOSURE_SIZE(nouters);
		LVNativeClosure *nc = (LVNativeClosure *)LV_MALLOC(size);
		new (nc) LVNativeClosure(ss, func);
		nc->_outervalues = (LVObjectPtr *)(nc + 1);
		nc->_noutervalues = nouters;
		_CONSTRUCT_VECTOR(LVObjectPtr, nc->_noutervalues, nc->_outervalues);
		return nc;
	}

	LVNativeClosure *Clone() {
		LVNativeClosure *ret = LVNativeClosure::Create(_opt_ss(this), _function, _noutervalues);
		ret->_env = _env;
		if (ret->_env) __ObjAddRef(ret->_env);
		ret->_name = _name;
		_COPY_VECTOR(ret->_outervalues, _outervalues, _noutervalues);
		ret->_typecheck.copy(_typecheck);
		ret->_nparamscheck = _nparamscheck;
		return ret;
	}

	~LVNativeClosure() {
		__ObjRelease(_env);
		REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain, this);
	}

	void Release() {
		LVInteger size = _CALC_NATVIVECLOSURE_SIZE(_noutervalues);
		_DESTRUCT_VECTOR(LVObjectPtr, _noutervalues, _outervalues);
		this->~LVNativeClosure();
		lv_free(this, size);
	}

#ifndef NO_GARBAGE_COLLECTOR
	void Mark(LVCollectable **chain);

	void Finalize() {
		_NULL_OBJECT_VECTOR(_outervalues, _noutervalues);
	}

	LVObjectType GetType() {
		return OT_NATIVECLOSURE;
	}
#endif

	LVInteger _nparamscheck;
	LVIntVector _typecheck;
	LVObjectPtr *_outervalues;
	LVUnsignedInteger _noutervalues;
	LVWeakRef *_env;
	LVFUNCTION _function;
	LVObjectPtr _name;
};

#endif // _LVCLOSURE_H_
