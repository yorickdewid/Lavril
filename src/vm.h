#ifndef _VM_H_
#define _VM_H_

#include "opcodes.h"
#include "object.h"

#define MAX_NATIVE_CALLS 100
#define MIN_STACK_OVERHEAD 15

#define SQ_SUSPEND_FLAG -666
#define DONT_FALL_BACK 666
//#define EXISTS_FALL_BACK -1

#define GET_FLAG_RAW                0x00000001
#define GET_FLAG_DO_NOT_RAISE_ERROR 0x00000002

void lv_base_register(VMHANDLE v);

struct SQExceptionTrap {
	SQExceptionTrap() {}
	SQExceptionTrap(LVInteger ss, LVInteger stackbase, SQInstruction *ip, LVInteger ex_target) {
		_stacksize = ss;
		_stackbase = stackbase;
		_ip = ip;
		_extarget = ex_target;
	}
	SQExceptionTrap(const SQExceptionTrap& et) {
		(*this) = et;
	}
	LVInteger _stackbase;
	LVInteger _stacksize;
	SQInstruction *_ip;
	LVInteger _extarget;
};

#define _INLINE

#define STK(a) _stack._vals[_stackbase+(a)]
#define TARGET _stack._vals[_stackbase+arg0]

typedef sqvector<SQExceptionTrap> ExceptionsTraps;

struct SQVM : public CHAINABLE_OBJ {
	struct CallInfo {
		//CallInfo() { _generator = NULL;}
		SQInstruction *_ip;
		SQObjectPtr *_literals;
		SQObjectPtr _closure;
		SQGenerator *_generator;
		LVInt32 _etraps;
		LVInt32 _prevstkbase;
		LVInt32 _prevtop;
		LVInt32 _target;
		LVInt32 _ncalls;
		LVBool _root;
	};

	typedef sqvector<CallInfo> CallInfoVec;
  public:
	void DebugHookProxy(LVInteger type, const LVChar *sourcename, LVInteger line, const LVChar *funcname);
	static void _DebugHookProxy(VMHANDLE v, LVInteger type, const LVChar *sourcename, LVInteger line, const LVChar *funcname);
	enum ExecutionType { ET_CALL, ET_RESUME_GENERATOR, ET_RESUME_VM, ET_RESUME_THROW_VM };
	SQVM(SQSharedState *ss);
	~SQVM();
	bool Init(SQVM *friendvm, LVInteger stacksize);
	bool Execute(SQObjectPtr& func, LVInteger nargs, LVInteger stackbase, SQObjectPtr& outres, LVBool raiseerror, ExecutionType et = ET_CALL);
	//starts a native call return when the NATIVE closure returns
	bool CallNative(SQNativeClosure *nclosure, LVInteger nargs, LVInteger newbase, SQObjectPtr& retval, bool& suspend);
	//starts a SQUIRREL call in the same "Execution loop"
	bool StartCall(SQClosure *closure, LVInteger target, LVInteger nargs, LVInteger stackbase, bool tailcall);
	bool CreateClassInstance(SQClass *theclass, SQObjectPtr& inst, SQObjectPtr& constructor);
	//call a generic closure pure SQUIRREL or NATIVE
	bool Call(SQObjectPtr& closure, LVInteger nparams, LVInteger stackbase, SQObjectPtr& outres, LVBool raiseerror);
	LVRESULT Suspend();

	void CallDebugHook(LVInteger type, LVInteger forcedline = 0);
	void CallErrorHandler(SQObjectPtr& e);
	bool Get(const SQObjectPtr& self, const SQObjectPtr& key, SQObjectPtr& dest, LVUnsignedInteger getflags, LVInteger selfidx);
	LVInteger FallBackGet(const SQObjectPtr& self, const SQObjectPtr& key, SQObjectPtr& dest);
	bool InvokeDefaultDelegate(const SQObjectPtr& self, const SQObjectPtr& key, SQObjectPtr& dest);
	bool Set(const SQObjectPtr& self, const SQObjectPtr& key, const SQObjectPtr& val, LVInteger selfidx);
	LVInteger FallBackSet(const SQObjectPtr& self, const SQObjectPtr& key, const SQObjectPtr& val);
	bool NewSlot(const SQObjectPtr& self, const SQObjectPtr& key, const SQObjectPtr& val, bool bstatic);
	bool NewSlotA(const SQObjectPtr& self, const SQObjectPtr& key, const SQObjectPtr& val, const SQObjectPtr& attrs, bool bstatic, bool raw);
	bool DeleteSlot(const SQObjectPtr& self, const SQObjectPtr& key, SQObjectPtr& res);
	bool Clone(const SQObjectPtr& self, SQObjectPtr& target);
	bool ObjCmp(const SQObjectPtr& o1, const SQObjectPtr& o2, LVInteger& res);
	bool StringCat(const SQObjectPtr& str, const SQObjectPtr& obj, SQObjectPtr& dest);
	static bool IsEqual(const SQObjectPtr& o1, const SQObjectPtr& o2, bool& res);
	bool ToString(const SQObjectPtr& o, SQObjectPtr& res, LVInteger ident = 0);
	SQString *PrintObjVal(const SQObjectPtr& o);

	/* Exception handeling */
	void Raise_Error(const LVChar *s, ...);
	void Raise_Error(const SQObjectPtr& desc);
	void Raise_IdxError(const SQObjectPtr& o);
	void Raise_CompareError(const SQObject& o1, const SQObject& o2);
	void Raise_ParamTypeError(LVInteger nparam, LVInteger typemask, LVInteger type);

	void FindOuter(SQObjectPtr& target, SQObjectPtr *stackindex);
	void RelocateOuters();
	void CloseOuters(SQObjectPtr *stackindex);

	bool TypeOf(const SQObjectPtr& obj1, SQObjectPtr& dest);
	bool CallMetaMethod(SQObjectPtr& closure, SQMetaMethod mm, LVInteger nparams, SQObjectPtr& outres);
	bool ArithMetaMethod(LVInteger op, const SQObjectPtr& o1, const SQObjectPtr& o2, SQObjectPtr& dest);
	bool Return(LVInteger _arg0, LVInteger _arg1, SQObjectPtr& retval);

	//new stuff
	_INLINE bool ARITH_OP(LVUnsignedInteger op, SQObjectPtr& trg, const SQObjectPtr& o1, const SQObjectPtr& o2);
	_INLINE bool BW_OP(LVUnsignedInteger op, SQObjectPtr& trg, const SQObjectPtr& o1, const SQObjectPtr& o2);
	_INLINE bool NEG_OP(SQObjectPtr& trg, const SQObjectPtr& o1);
	_INLINE bool CMP_OP(CmpOP op, const SQObjectPtr& o1, const SQObjectPtr& o2, SQObjectPtr& res);
	bool CLOSURE_OP(SQObjectPtr& target, FunctionPrototype *func);
	bool CLASS_OP(SQObjectPtr& target, LVInteger base, LVInteger attrs);
	//return true if the loop is finished
	bool FOREACH_OP(SQObjectPtr& o1, SQObjectPtr& o2, SQObjectPtr& o3, SQObjectPtr& o4, LVInteger arg_2, int exitpos, int& jump);
	//_INLINE bool LOCAL_INC(LVInteger op,SQObjectPtr &target, SQObjectPtr &a, SQObjectPtr &incr);
	_INLINE bool PLOCAL_INC(LVInteger op, SQObjectPtr& target, SQObjectPtr& a, SQObjectPtr& incr);
	_INLINE bool DerefInc(LVInteger op, SQObjectPtr& target, SQObjectPtr& self, SQObjectPtr& key, SQObjectPtr& incr, bool postfix, LVInteger arg0);
	// #ifdef _DEBUG_DUMP
	void dumpstack(LVInteger stackbase = -1, bool dumpall = false);
	// #endif

#ifndef NO_GARBAGE_COLLECTOR
	void Mark(SQCollectable **chain);
	SQObjectType GetType() {
		return OT_THREAD;
	}
#endif
	void Finalize();
	void GrowCallStack() {
		LVInteger newsize = _alloccallsstacksize * 2;
		_callstackdata.resize(newsize);
		_callsstack = &_callstackdata[0];
		_alloccallsstacksize = newsize;
	}
	bool EnterFrame(LVInteger newbase, LVInteger newtop, bool tailcall);
	void LeaveFrame();
	void Release() {
		sq_delete(this, SQVM);
	}
	////////////////////////////////////////////////////////////////////////////
	//stack functions for the api
	void Remove(LVInteger n);

	static bool IsFalse(SQObjectPtr& o);

	void Pop();
	void Pop(LVInteger n);
	void Push(const SQObjectPtr& o);
	void PushNull();
	SQObjectPtr& Top();
	SQObjectPtr& PopGet();
	SQObjectPtr& GetUp(LVInteger n);
	SQObjectPtr& GetAt(LVInteger n);

	SQObjectPtrVec _stack;

	LVInteger _top;
	LVInteger _stackbase;
	SQOuter *_openouters;
	SQObjectPtr _roottable;
	SQObjectPtr _lasterror;
	SQObjectPtr _errorhandler;

	bool _debughook;
	SQDEBUGHOOK _debughook_native;
	SQObjectPtr _debughook_closure;

	SQObjectPtr temp_reg;

	/* Stacks */
	CallInfo *_callsstack;
	LVInteger _callsstacksize;
	LVInteger _alloccallsstacksize;
	sqvector<CallInfo> _callstackdata;

	ExceptionsTraps _etraps;
	CallInfo *ci;
	LVUserPointer _foreignptr;

	/* VMs sharing the same state */
	SQSharedState *_sharedstate;
	LVInteger _nnativecalls;
	LVInteger _nmetamethodscall;
	SQRELEASEHOOK _releasehook;

	/* Suspend infos */
	LVBool _suspended;
	LVBool _suspended_root;
	LVInteger _suspended_target;
	LVInteger _suspended_traps;
};

struct AutoDec {
	AutoDec(LVInteger *n) {
		_n = n;
	}
	~AutoDec() {
		(*_n)--;
	}
	LVInteger *_n;
};

inline SQObjectPtr& stack_get(VMHANDLE v, LVInteger idx) {
	return ((idx >= 0) ? (v->GetAt(idx + v->_stackbase - 1)) : (v->GetUp(idx)));
}

#define _ss(_vm_) (_vm_)->_sharedstate

#ifndef NO_GARBAGE_COLLECTOR
#define _opt_ss(_vm_) (_vm_)->_sharedstate
#else
#define _opt_ss(_vm_) NULL
#endif

#define PUSH_CALLINFO(v,nci){ \
    LVInteger css = v->_callsstacksize; \
    if(css == v->_alloccallsstacksize) { \
        v->GrowCallStack(); \
    } \
    v->ci = &v->_callsstack[css]; \
    *(v->ci) = nci; \
    v->_callsstacksize++; \
}

#define POP_CALLINFO(v){ \
    LVInteger css = --v->_callsstacksize; \
    v->ci->_closure.Null(); \
    v->ci = css?&v->_callsstack[css-1]:NULL;    \
}
#endif // _VM_H_
