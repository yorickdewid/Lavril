#ifndef _VM_H_
#define _VM_H_

#include "opcodes.h"
#include "object.h"

#define MAX_NATIVE_CALLS 100
#define MIN_STACK_OVERHEAD 15

#define SUSPEND_FLAG -666
#define DONT_FALL_BACK 666
//#define EXISTS_FALL_BACK -1

#define GET_FLAG_RAW                0x00000001
#define GET_FLAG_DO_NOT_RAISE_ERROR 0x00000002

void lv_base_register(VMHANDLE v);

struct LVExceptionTrap {
	LVExceptionTrap() {}
	LVExceptionTrap(LVInteger ss, LVInteger stackbase, LVInstruction *ip, LVInteger ex_target) {
		_stacksize = ss;
		_stackbase = stackbase;
		_ip = ip;
		_extarget = ex_target;
	}
	LVExceptionTrap(const LVExceptionTrap& et) {
		(*this) = et;
	}
	LVInteger _stackbase;
	LVInteger _stacksize;
	LVInstruction *_ip;
	LVInteger _extarget;
};

#define _INLINE

#define STK(a) _stack._vals[_stackbase+(a)]
#define TARGET _stack._vals[_stackbase+arg0]

typedef lvvector<LVExceptionTrap> ExceptionsTraps;

struct LVVM : public CHAINABLE_OBJ {
	struct CallInfo {
		//CallInfo() { _generator = NULL;}
		LVInstruction *_ip;
		LVObjectPtr *_literals;
		LVObjectPtr _closure;
		LVGenerator *_generator;
		LVInt32 _etraps;
		LVInt32 _prevstkbase;
		LVInt32 _prevtop;
		LVInt32 _target;
		LVInt32 _ncalls;
		LVBool _root;
	};

	typedef lvvector<CallInfo> CallInfoVec;
  public:
	void DebugHookProxy(LVInteger type, const LVChar *sourcename, LVInteger line, const LVChar *funcname);
	static void _DebugHookProxy(VMHANDLE v, LVInteger type, const LVChar *sourcename, LVInteger line, const LVChar *funcname);
	enum ExecutionType { ET_CALL, ET_RESUME_GENERATOR, ET_RESUME_VM, ET_RESUME_THROW_VM };
	LVVM(LVSharedState *ss);
	~LVVM();
	bool Init(LVVM *friendvm, LVInteger stacksize);
	bool Execute(LVObjectPtr& func, LVInteger nargs, LVInteger stackbase, LVObjectPtr& outres, LVBool raiseerror, ExecutionType et = ET_CALL);
	bool CallNative(LVNativeClosure *nclosure, LVInteger nargs, LVInteger newbase, LVObjectPtr& retval, bool& suspend);
	bool StartCall(LVClosure *closure, LVInteger target, LVInteger nargs, LVInteger stackbase, bool tailcall);
	bool CreateClassInstance(LVClass *theclass, LVObjectPtr& inst, LVObjectPtr& constructor);
	bool Call(LVObjectPtr& closure, LVInteger nparams, LVInteger stackbase, LVObjectPtr& outres, LVBool raiseerror);
	LVRESULT Suspend();

	void CallDebugHook(LVInteger type, LVInteger forcedline = 0);
	void CallErrorHandler(LVObjectPtr& e);
	bool Get(const LVObjectPtr& self, const LVObjectPtr& key, LVObjectPtr& dest, LVUnsignedInteger getflags, LVInteger selfidx);
	LVInteger FallBackGet(const LVObjectPtr& self, const LVObjectPtr& key, LVObjectPtr& dest);
	bool InvokeDefaultDelegate(const LVObjectPtr& self, const LVObjectPtr& key, LVObjectPtr& dest);
	bool Set(const LVObjectPtr& self, const LVObjectPtr& key, const LVObjectPtr& val, LVInteger selfidx);
	LVInteger FallBackSet(const LVObjectPtr& self, const LVObjectPtr& key, const LVObjectPtr& val);
	bool NewSlot(const LVObjectPtr& self, const LVObjectPtr& key, const LVObjectPtr& val, bool bstatic);
	bool NewSlotA(const LVObjectPtr& self, const LVObjectPtr& key, const LVObjectPtr& val, const LVObjectPtr& attrs, bool bstatic, bool raw);
	bool DeleteSlot(const LVObjectPtr& self, const LVObjectPtr& key, LVObjectPtr& res);
	bool Clone(const LVObjectPtr& self, LVObjectPtr& target);
	bool ObjCmp(const LVObjectPtr& o1, const LVObjectPtr& o2, LVInteger& res);
	bool StringCat(const LVObjectPtr& str, const LVObjectPtr& obj, LVObjectPtr& dest);
	static bool IsEqual(const LVObjectPtr& o1, const LVObjectPtr& o2, bool& res);
	bool ToString(const LVObjectPtr& o, LVObjectPtr& res, LVInteger ident = 0);
	LVString *PrintObjVal(const LVObjectPtr& o);

	/* Exception handeling */
	void Raise_Error(const LVChar *s, ...);
	void Raise_Error(const LVObjectPtr& desc);
	void Raise_IdxError(const LVObjectPtr& o);
	void Raise_CompareError(const LVObject& o1, const LVObject& o2);
	void Raise_ParamTypeError(LVInteger nparam, LVInteger typemask, LVInteger type);

	void FindOuter(LVObjectPtr& target, LVObjectPtr *stackindex);
	void RelocateOuters();
	void CloseOuters(LVObjectPtr *stackindex);

	bool TypeOf(const LVObjectPtr& obj1, LVObjectPtr& dest);
	bool CallMetaMethod(LVObjectPtr& closure, LVMetaMethod mm, LVInteger nparams, LVObjectPtr& outres);
	bool ArithMetaMethod(LVInteger op, const LVObjectPtr& o1, const LVObjectPtr& o2, LVObjectPtr& dest);
	bool Return(LVInteger _arg0, LVInteger _arg1, LVObjectPtr& retval);

	//new stuff
	_INLINE bool ARITH_OP(LVUnsignedInteger op, LVObjectPtr& trg, const LVObjectPtr& o1, const LVObjectPtr& o2);
	_INLINE bool BW_OP(LVUnsignedInteger op, LVObjectPtr& trg, const LVObjectPtr& o1, const LVObjectPtr& o2);
	_INLINE bool NEG_OP(LVObjectPtr& trg, const LVObjectPtr& o1);
	_INLINE bool CMP_OP(CmpOP op, const LVObjectPtr& o1, const LVObjectPtr& o2, LVObjectPtr& res);
	bool CLOSURE_OP(LVObjectPtr& target, FunctionPrototype *func);
	bool CLASS_OP(LVObjectPtr& target, LVInteger base, LVInteger attrs);
	//return true if the loop is finished
	bool FOREACH_OP(LVObjectPtr& o1, LVObjectPtr& o2, LVObjectPtr& o3, LVObjectPtr& o4, LVInteger arg_2, int exitpos, int& jump);
	//_INLINE bool LOCAL_INC(LVInteger op,LVObjectPtr &target, LVObjectPtr &a, LVObjectPtr &incr);
	_INLINE bool PLOCAL_INC(LVInteger op, LVObjectPtr& target, LVObjectPtr& a, LVObjectPtr& incr);
	_INLINE bool DerefInc(LVInteger op, LVObjectPtr& target, LVObjectPtr& self, LVObjectPtr& key, LVObjectPtr& incr, bool postfix, LVInteger arg0);
	// #ifdef _DEBUG_DUMP
	void dumpstack(LVInteger stackbase = -1, bool dumpall = false);
	// #endif

#ifndef NO_GARBAGE_COLLECTOR
	void Mark(LVCollectable **chain);
	LVObjectType GetType() {
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
		lv_delete(this, LVVM);
	}
	////////////////////////////////////////////////////////////////////////////
	//stack functions for the api
	void Remove(LVInteger n);

	static bool IsFalse(LVObjectPtr& o);

	void Pop();
	void Pop(LVInteger n);
	void Push(const LVObjectPtr& o);
	void PushNull();
	LVObjectPtr& Top();
	LVObjectPtr& PopGet();
	LVObjectPtr& GetUp(LVInteger n);
	LVObjectPtr& GetAt(LVInteger n);

	LVObjectPtrVec _stack;

	LVInteger _top;
	LVInteger _stackbase;
	LVOuter *_openouters;
	LVObjectPtr _roottable;
	LVObjectPtr _lasterror;
	LVObjectPtr _errorhandler;

	bool _debughook;
	LVDEBUGHOOK _debughook_native;
	LVObjectPtr _debughook_closure;

	LVObjectPtr temp_reg;

	/* Stacks */
	CallInfo *_callsstack;
	LVInteger _callsstacksize;
	LVInteger _alloccallsstacksize;
	lvvector<CallInfo> _callstackdata;

	ExceptionsTraps _etraps;
	CallInfo *ci;
	LVUserPointer _foreignptr;

	/* VMs sharing the same state */
	LVSharedState *_sharedstate;
	LVInteger _nnativecalls;
	LVInteger _nmetamethodscall;
	LVRELEASEHOOK _releasehook;

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

inline LVObjectPtr& stack_get(VMHANDLE v, LVInteger idx) {
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
