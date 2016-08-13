#ifndef _FUNCTIONSTATE_H_
#define _FUNCTIONSTATE_H_

#include "utils.h"

struct FunctionState {
	FunctionState(SQSharedState *ss, FunctionState *parent, CompilerErrorFunc efunc, void *ed);
	~FunctionState();
	// #ifdef _DEBUG_DUMP
	void Dump(FunctionPrototype *func);
	// #endif
	void Error(const LVChar *err);
	FunctionState *PushChildState(SQSharedState *ss);
	void PopChildState();
	void AddInstruction(SQOpcode _op, LVInteger arg0 = 0, LVInteger arg1 = 0, LVInteger arg2 = 0, LVInteger arg3 = 0) {
		SQInstruction i(_op, arg0, arg1, arg2, arg3);
		AddInstruction(i);
	}
	void AddInstruction(SQInstruction& i);
	void SetIntructionParams(LVInteger pos, LVInteger arg0, LVInteger arg1, LVInteger arg2 = 0, LVInteger arg3 = 0);
	void SetIntructionParam(LVInteger pos, LVInteger arg, LVInteger val);
	SQInstruction& GetInstruction(LVInteger pos) {
		return _instructions[pos];
	}
	void PopInstructions(LVInteger size) {
		for (LVInteger i = 0; i < size; i++)_instructions.pop_back();
	}
	void SetStackSize(LVInteger n);
	LVInteger CountOuters(LVInteger stacksize);
	void SnoozeOpt() {
		_optimization = false;
	}
	void AddDefaultParam(LVInteger trg) {
		_defaultparams.push_back(trg);
	}
	LVInteger GetDefaultParamCount() {
		return _defaultparams.size();
	}
	LVInteger GetCurrentPos() {
		return _instructions.size() - 1;
	}
	LVInteger GetNumericConstant(const LVInteger cons);
	LVInteger GetNumericConstant(const LVFloat cons);
	LVInteger PushLocalVariable(const SQObject& name);
	void AddParameter(const SQObject& name);
	//void AddOuterValue(const SQObject &name);
	LVInteger GetLocalVariable(const SQObject& name);
	void MarkLocalAsOuter(LVInteger pos);
	LVInteger GetOuterVariable(const SQObject& name);
	LVInteger GenerateCode();
	LVInteger GetStackSize();
	LVInteger CalcStackFrameSize();
	void AddLineInfos(LVInteger line, bool lineop, bool force = false);
	FunctionPrototype *BuildProto();
	LVInteger AllocStackPos();
	LVInteger PushTarget(LVInteger n = -1);
	LVInteger PopTarget();
	LVInteger TopTarget();
	LVInteger GetUpTarget(LVInteger n);
	void DiscardTarget();
	bool IsLocal(LVUnsignedInteger stkpos);
	SQObject CreateString(const LVChar *s, LVInteger len = -1);
	SQObject CreateTable();
	bool IsConstant(const SQObject& name, SQObject& e);
	LVInteger _returnexp;
	SQLocalVarInfoVec _vlocals;
	SQIntVec _targetstack;
	LVInteger _stacksize;
	bool _varparams;
	bool _bgenerator;
	SQIntVec _unresolvedbreaks;
	SQIntVec _unresolvedcontinues;
	SQObjectPtrVec _functions;
	SQObjectPtrVec _parameters;
	SQOuterVarVec _outervalues;
	SQInstructionVec _instructions;
	SQLocalVarInfoVec _localvarinfos;
	SQObjectPtr _literals;
	SQObjectPtr _strings;
	SQObjectPtr _name;
	SQObjectPtr _sourcename; /* Name of unit */
	LVInteger _nliterals;
	SQLineInfoVec _lineinfos;
	FunctionState *_parent;
	SQIntVec _scope_blocks;
	SQIntVec _breaktargets;
	SQIntVec _continuetargets;
	SQIntVec _defaultparams;
	LVInteger _lastline;
	LVInteger _traps; /* Contains number of nested exception traps */
	LVInteger _outers;
	bool _optimization;
	SQSharedState *_sharedstate;
	sqvector<FunctionState *> _childstates;
	LVInteger GetConstant(const SQObject& cons);

  private:
	CompilerErrorFunc _errfunc;
	void *_errtarget;
	SQSharedState *_ss;
};

#endif // _FUNCTIONSTATE_H_

