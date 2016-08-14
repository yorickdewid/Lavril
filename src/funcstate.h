#ifndef _FUNCTIONSTATE_H_
#define _FUNCTIONSTATE_H_

#include "utils.h"

struct FunctionState {
	FunctionState(LVSharedState *ss, FunctionState *parent, CompilerErrorFunc efunc, void *ed);
	~FunctionState();
	// #ifdef _DEBUG_DUMP
	void Dump(FunctionPrototype *func);
	// #endif
	void Error(const LVChar *err);
	FunctionState *PushChildState(LVSharedState *ss);
	void PopChildState();
	void AddInstruction(Opcode _op, LVInteger arg0 = 0, LVInteger arg1 = 0, LVInteger arg2 = 0, LVInteger arg3 = 0) {
		LVInstruction i(_op, arg0, arg1, arg2, arg3);
		AddInstruction(i);
	}
	void AddInstruction(LVInstruction& i);
	void SetIntructionParams(LVInteger pos, LVInteger arg0, LVInteger arg1, LVInteger arg2 = 0, LVInteger arg3 = 0);
	void SetIntructionParam(LVInteger pos, LVInteger arg, LVInteger val);
	LVInstruction& GetInstruction(LVInteger pos) {
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
	LVInteger PushLocalVariable(const LVObject& name);
	void AddParameter(const LVObject& name);
	//void AddOuterValue(const LVObject &name);
	LVInteger GetLocalVariable(const LVObject& name);
	void MarkLocalAsOuter(LVInteger pos);
	LVInteger GetOuterVariable(const LVObject& name);
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
	LVObject CreateString(const LVChar *s, LVInteger len = -1);
	LVObject CreateTable();
	bool IsConstant(const LVObject& name, LVObject& e);
	LVInteger _returnexp;
	LVLocalVarInfoVec _vlocals;
	LVIntVector _targetstack;
	LVInteger _stacksize;
	bool _varparams;
	bool _bgenerator;
	LVIntVector _unresolvedbreaks;
	LVIntVector _unresolvedcontinues;
	LVObjectPtrVec _functions;
	LVObjectPtrVec _parameters;
	LVOuterVarVec _outervalues;
	LVInstructionVec _instructions;
	LVLocalVarInfoVec _localvarinfos;
	LVObjectPtr _literals;
	LVObjectPtr _strings;
	LVObjectPtr _name;
	LVObjectPtr _sourcename; /* Name of unit */
	LVInteger _nliterals;
	LVLineInfoVec _lineinfos;
	FunctionState *_parent;
	LVIntVector _scope_blocks;
	LVIntVector _breaktargets;
	LVIntVector _continuetargets;
	LVIntVector _defaultparams;
	LVInteger _lastline;
	LVInteger _traps; /* Contains number of nested exception traps */
	LVInteger _outers;
	bool _optimization;
	LVSharedState *_sharedstate;
	lvvector<FunctionState *> _childstates;
	LVInteger GetConstant(const LVObject& cons);

  private:
	CompilerErrorFunc _errfunc;
	void *_errtarget;
	LVSharedState *_ss;
};

#endif // _FUNCTIONSTATE_H_

