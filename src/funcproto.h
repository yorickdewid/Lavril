#ifndef _FUNCTION_H_
#define _FUNCTION_H_

#include "opcodes.h"

enum SQOuterType {
	otLOCAL = 0,
	otOUTER = 1
};

struct SQOuterVar {
	SQOuterVar() {}
	SQOuterVar(const SQObjectPtr& name, const SQObjectPtr& src, SQOuterType t) {
		_name = name;
		_src = src;
		_type = t;
	}

	SQOuterVar(const SQOuterVar& ov) {
		_type = ov._type;
		_src = ov._src;
		_name = ov._name;
	}

	SQOuterType _type;
	SQObjectPtr _name;
	SQObjectPtr _src;
};

struct SQLocalVarInfo {
	SQLocalVarInfo(): _start_op(0), _end_op(0), _pos(0) {}
	SQLocalVarInfo(const SQLocalVarInfo& lvi) {
		_name = lvi._name;
		_start_op = lvi._start_op;
		_end_op = lvi._end_op;
		_pos = lvi._pos;
	}

	SQObjectPtr _name;
	LVUnsignedInteger _start_op;
	LVUnsignedInteger _end_op;
	LVUnsignedInteger _pos;
};

struct SQLineInfo {
	LVInteger _line;
	LVInteger _op;
};

typedef sqvector<SQOuterVar> SQOuterVarVec;
typedef sqvector<SQLocalVarInfo> SQLocalVarInfoVec;
typedef sqvector<SQLineInfo> SQLineInfoVec;

#define _FUNC_SIZE(ni,nl,nparams,nfuncs,nouters,nlineinf,localinf,defparams) (sizeof(FunctionPrototype) \
        +((ni-1)*sizeof(SQInstruction))+(nl*sizeof(SQObjectPtr)) \
        +(nparams*sizeof(SQObjectPtr))+(nfuncs*sizeof(SQObjectPtr)) \
        +(nouters*sizeof(SQOuterVar))+(nlineinf*sizeof(SQLineInfo)) \
        +(localinf*sizeof(SQLocalVarInfo))+(defparams*sizeof(LVInteger)))


struct FunctionPrototype : public CHAINABLE_OBJ {
  private:
	FunctionPrototype(SQSharedState *ss);
	~FunctionPrototype();

  public:
	static FunctionPrototype *Create(SQSharedState *ss, LVInteger ninstructions,
	                                 LVInteger nliterals, LVInteger nparameters,
	                                 LVInteger nfunctions, LVInteger noutervalues,
	                                 LVInteger nlineinfos, LVInteger nlocalvarinfos, LVInteger ndefaultparams) {
		FunctionPrototype *f;
		//I compact the whole class and members in a single memory allocation
		f = (FunctionPrototype *)lv_vm_malloc(_FUNC_SIZE(ninstructions, nliterals, nparameters, nfunctions, noutervalues, nlineinfos, nlocalvarinfos, ndefaultparams));
		new (f) FunctionPrototype(ss);
		f->_ninstructions = ninstructions;
		f->_literals = (SQObjectPtr *)&f->_instructions[ninstructions];
		f->_nliterals = nliterals;
		f->_parameters = (SQObjectPtr *)&f->_literals[nliterals];
		f->_nparameters = nparameters;
		f->_functions = (SQObjectPtr *)&f->_parameters[nparameters];
		f->_nfunctions = nfunctions;
		f->_outervalues = (SQOuterVar *)&f->_functions[nfunctions];
		f->_noutervalues = noutervalues;
		f->_lineinfos = (SQLineInfo *)&f->_outervalues[noutervalues];
		f->_nlineinfos = nlineinfos;
		f->_localvarinfos = (SQLocalVarInfo *)&f->_lineinfos[nlineinfos];
		f->_nlocalvarinfos = nlocalvarinfos;
		f->_defaultparams = (LVInteger *)&f->_localvarinfos[nlocalvarinfos];
		f->_ndefaultparams = ndefaultparams;

		_CONSTRUCT_VECTOR(SQObjectPtr, f->_nliterals, f->_literals);
		_CONSTRUCT_VECTOR(SQObjectPtr, f->_nparameters, f->_parameters);
		_CONSTRUCT_VECTOR(SQObjectPtr, f->_nfunctions, f->_functions);
		_CONSTRUCT_VECTOR(SQOuterVar, f->_noutervalues, f->_outervalues);
		//_CONSTRUCT_VECTOR(SQLineInfo,f->_nlineinfos,f->_lineinfos); //not required are 2 integers
		_CONSTRUCT_VECTOR(SQLocalVarInfo, f->_nlocalvarinfos, f->_localvarinfos);
		return f;
	}

	void Release() {
		_DESTRUCT_VECTOR(SQObjectPtr, _nliterals, _literals);
		_DESTRUCT_VECTOR(SQObjectPtr, _nparameters, _parameters);
		_DESTRUCT_VECTOR(SQObjectPtr, _nfunctions, _functions);
		_DESTRUCT_VECTOR(SQOuterVar, _noutervalues, _outervalues);
		//_DESTRUCT_VECTOR(SQLineInfo,_nlineinfos,_lineinfos); //not required are 2 integers
		_DESTRUCT_VECTOR(SQLocalVarInfo, _nlocalvarinfos, _localvarinfos);
		LVInteger size = _FUNC_SIZE(_ninstructions, _nliterals, _nparameters, _nfunctions, _noutervalues, _nlineinfos, _nlocalvarinfos, _ndefaultparams);
		this->~FunctionPrototype();
		lv_vm_free(this, size);
	}

	const LVChar *GetLocal(SQVM *v, LVUnsignedInteger stackbase, LVUnsignedInteger nseq, LVUnsignedInteger nop);
	LVInteger GetLine(SQInstruction *curr);
	bool Save(SQVM *v, LVUserPointer up, SQWRITEFUNC write);
	static bool Load(SQVM *v, LVUserPointer up, SQREADFUNC read, SQObjectPtr& ret);
#ifndef NO_GARBAGE_COLLECTOR
	void Mark(SQCollectable **chain);
	void Finalize() {
		_NULL_SQOBJECT_VECTOR(_literals, _nliterals);
	}
	SQObjectType GetType() {
		return OT_FUNCPROTO;
	}
#endif

	SQObjectPtr _sourcename;
	SQObjectPtr _name;
	LVInteger _stacksize;
	bool _bgenerator;
	LVInteger _varparams;

	LVInteger _nlocalvarinfos;
	SQLocalVarInfo *_localvarinfos;

	LVInteger _nlineinfos;
	SQLineInfo *_lineinfos;

	LVInteger _nliterals;
	SQObjectPtr *_literals;

	LVInteger _nparameters;
	SQObjectPtr *_parameters;

	LVInteger _nfunctions;
	SQObjectPtr *_functions;

	LVInteger _noutervalues;
	SQOuterVar *_outervalues;

	LVInteger _ndefaultparams;
	LVInteger *_defaultparams;

	LVInteger _ninstructions;
	SQInstruction _instructions[1];
};

#endif // _FUNCTION_H_
