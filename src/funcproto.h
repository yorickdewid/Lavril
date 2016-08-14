#ifndef _FUNCTION_H_
#define _FUNCTION_H_

#include "opcodes.h"

enum LVOuterType {
	otLOCAL = 0,
	otOUTER = 1
};

struct LVOuterVar {
	LVOuterVar() {}
	LVOuterVar(const LVObjectPtr& name, const LVObjectPtr& src, LVOuterType t) {
		_name = name;
		_src = src;
		_type = t;
	}

	LVOuterVar(const LVOuterVar& ov) {
		_type = ov._type;
		_src = ov._src;
		_name = ov._name;
	}

	LVOuterType _type;
	LVObjectPtr _name;
	LVObjectPtr _src;
};

struct LVLocalVarInfo {
	LVLocalVarInfo(): _start_op(0), _end_op(0), _pos(0) {}
	LVLocalVarInfo(const LVLocalVarInfo& lvi) {
		_name = lvi._name;
		_start_op = lvi._start_op;
		_end_op = lvi._end_op;
		_pos = lvi._pos;
	}

	LVObjectPtr _name;
	LVUnsignedInteger _start_op;
	LVUnsignedInteger _end_op;
	LVUnsignedInteger _pos;
};

struct LVLineInfo {
	LVInteger _line;
	LVInteger _op;
};

typedef lvvector<LVOuterVar> LVOuterVarVec;
typedef lvvector<LVLocalVarInfo> LVLocalVarInfoVec;
typedef lvvector<LVLineInfo> LVLineInfoVec;

#define _FUNC_SIZE(ni,nl,nparams,nfuncs,nouters,nlineinf,localinf,defparams) (sizeof(FunctionPrototype) \
        +((ni-1)*sizeof(LVInstruction))+(nl*sizeof(LVObjectPtr)) \
        +(nparams*sizeof(LVObjectPtr))+(nfuncs*sizeof(LVObjectPtr)) \
        +(nouters*sizeof(LVOuterVar))+(nlineinf*sizeof(LVLineInfo)) \
        +(localinf*sizeof(LVLocalVarInfo))+(defparams*sizeof(LVInteger)))


struct FunctionPrototype : public CHAINABLE_OBJ {
  private:
	FunctionPrototype(LVSharedState *ss);
	~FunctionPrototype();

  public:
	static FunctionPrototype *Create(LVSharedState *ss, LVInteger ninstructions,
	                                 LVInteger nliterals, LVInteger nparameters,
	                                 LVInteger nfunctions, LVInteger noutervalues,
	                                 LVInteger nlineinfos, LVInteger nlocalvarinfos, LVInteger ndefaultparams) {
		FunctionPrototype *f;
		//I compact the whole class and members in a single memory allocation
		f = (FunctionPrototype *)lv_vm_malloc(_FUNC_SIZE(ninstructions, nliterals, nparameters, nfunctions, noutervalues, nlineinfos, nlocalvarinfos, ndefaultparams));
		new (f) FunctionPrototype(ss);
		f->_ninstructions = ninstructions;
		f->_literals = (LVObjectPtr *)&f->_instructions[ninstructions];
		f->_nliterals = nliterals;
		f->_parameters = (LVObjectPtr *)&f->_literals[nliterals];
		f->_nparameters = nparameters;
		f->_functions = (LVObjectPtr *)&f->_parameters[nparameters];
		f->_nfunctions = nfunctions;
		f->_outervalues = (LVOuterVar *)&f->_functions[nfunctions];
		f->_noutervalues = noutervalues;
		f->_lineinfos = (LVLineInfo *)&f->_outervalues[noutervalues];
		f->_nlineinfos = nlineinfos;
		f->_localvarinfos = (LVLocalVarInfo *)&f->_lineinfos[nlineinfos];
		f->_nlocalvarinfos = nlocalvarinfos;
		f->_defaultparams = (LVInteger *)&f->_localvarinfos[nlocalvarinfos];
		f->_ndefaultparams = ndefaultparams;

		_CONSTRUCT_VECTOR(LVObjectPtr, f->_nliterals, f->_literals);
		_CONSTRUCT_VECTOR(LVObjectPtr, f->_nparameters, f->_parameters);
		_CONSTRUCT_VECTOR(LVObjectPtr, f->_nfunctions, f->_functions);
		_CONSTRUCT_VECTOR(LVOuterVar, f->_noutervalues, f->_outervalues);
		//_CONSTRUCT_VECTOR(LVLineInfo,f->_nlineinfos,f->_lineinfos); //not required are 2 integers
		_CONSTRUCT_VECTOR(LVLocalVarInfo, f->_nlocalvarinfos, f->_localvarinfos);
		return f;
	}

	void Release() {
		_DESTRUCT_VECTOR(LVObjectPtr, _nliterals, _literals);
		_DESTRUCT_VECTOR(LVObjectPtr, _nparameters, _parameters);
		_DESTRUCT_VECTOR(LVObjectPtr, _nfunctions, _functions);
		_DESTRUCT_VECTOR(LVOuterVar, _noutervalues, _outervalues);
		//_DESTRUCT_VECTOR(LVLineInfo,_nlineinfos,_lineinfos); //not required are 2 integers
		_DESTRUCT_VECTOR(LVLocalVarInfo, _nlocalvarinfos, _localvarinfos);
		LVInteger size = _FUNC_SIZE(_ninstructions, _nliterals, _nparameters, _nfunctions, _noutervalues, _nlineinfos, _nlocalvarinfos, _ndefaultparams);
		this->~FunctionPrototype();
		lv_vm_free(this, size);
	}

	const LVChar *GetLocal(LVVM *v, LVUnsignedInteger stackbase, LVUnsignedInteger nseq, LVUnsignedInteger nop);
	LVInteger GetLine(LVInstruction *curr);
	bool Save(LVVM *v, LVUserPointer up, LVWRITEFUNC write);
	static bool Load(LVVM *v, LVUserPointer up, LVREADFUNC read, LVObjectPtr& ret);
#ifndef NO_GARBAGE_COLLECTOR
	void Mark(LVCollectable **chain);
	void Finalize() {
		_NULL_OBJECT_VECTOR(_literals, _nliterals);
	}
	LVObjectType GetType() {
		return OT_FUNCPROTO;
	}
#endif

	LVObjectPtr _sourcename;
	LVObjectPtr _name;
	LVInteger _stacksize;
	bool _bgenerator;
	LVInteger _varparams;

	LVInteger _nlocalvarinfos;
	LVLocalVarInfo *_localvarinfos;

	LVInteger _nlineinfos;
	LVLineInfo *_lineinfos;

	LVInteger _nliterals;
	LVObjectPtr *_literals;

	LVInteger _nparameters;
	LVObjectPtr *_parameters;

	LVInteger _nfunctions;
	LVObjectPtr *_functions;

	LVInteger _noutervalues;
	LVOuterVar *_outervalues;

	LVInteger _ndefaultparams;
	LVInteger *_defaultparams;

	LVInteger _ninstructions;
	LVInstruction _instructions[1];
};

#endif // _FUNCTION_H_
