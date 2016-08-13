#ifndef _STATE_H_
#define _STATE_H_

#include "utils.h"
#include "object.h"

struct SQString;
struct SQTable;

/* Max number of character for a printed number */
#define NUMBER_MAX_CHAR 50

struct SQStringTable {
	SQStringTable(SQSharedState *ss);
	~SQStringTable();
	SQString *Add(const LVChar *, LVInteger len);
	void Remove(SQString *);

  private:
	void Resize(LVInteger size);
	void AllocNodes(LVInteger size);
	SQString **_strings;
	LVUnsignedInteger _numofslots;
	LVUnsignedInteger _slotused;
	SQSharedState *_sharedstate;
};

struct RefTable {
	struct RefNode {
		SQObjectPtr obj;
		LVUnsignedInteger refs;
		struct RefNode *next;
	};
	RefTable();
	~RefTable();
	void AddRef(SQObject& obj);
	LVBool Release(SQObject& obj);
	LVUnsignedInteger GetRefCount(SQObject& obj);
#ifndef NO_GARBAGE_COLLECTOR
	void Mark(SQCollectable **chain);
#endif
	void Finalize();
  private:
	RefNode *Get(SQObject& obj, LVHash& mainpos, RefNode **prev, bool add);
	RefNode *Add(LVHash mainpos, SQObject& obj);
	void Resize(LVUnsignedInteger size);
	void AllocNodes(LVUnsignedInteger size);
	LVUnsignedInteger _numofslots;
	LVUnsignedInteger _slotused;
	RefNode *_nodes;
	RefNode *_freelist;
	RefNode **_buckets;
};

#define ADD_STRING(ss,str,len) ss->_stringtable->Add(str,len)
#define REMOVE_STRING(ss,bstr) ss->_stringtable->Remove(bstr)

struct SQObjectPtr;

struct SQSharedState {
	SQSharedState();
	~SQSharedState();
	void Init();

  public:
	LVChar *GetScratchPad(LVInteger size);
	LVInteger GetMetaMethodIdxByName(const SQObjectPtr& name);
#ifndef NO_GARBAGE_COLLECTOR
	LVInteger CollectGarbage(SQVM *vm);
	void RunMark(SQVM *vm, SQCollectable **tchain);
	LVInteger ResurrectUnreachable(SQVM *vm);
	static void MarkObject(SQObjectPtr& o, SQCollectable **chain);
#endif
	SQObjectPtrVec *_metamethods;
	SQObjectPtr _metamethodsmap;
	SQObjectPtrVec *_systemstrings;
	SQObjectPtrVec *_types;
	SQStringTable *_stringtable;
	RefTable _refs_table;
	SQObjectPtr _registry;
	SQObjectPtr _consts;
	SQObjectPtr _constructoridx;
#ifndef NO_GARBAGE_COLLECTOR
	SQCollectable *_gc_chain;
#endif
	SQObjectPtr _root_vm;
	SQObjectPtr _table_default_delegate;
	static const SQRegFunction _table_default_delegate_funcz[];
	SQObjectPtr _array_default_delegate;
	static const SQRegFunction _array_default_delegate_funcz[];
	SQObjectPtr _string_default_delegate;
	static const SQRegFunction _string_default_delegate_funcz[];
	SQObjectPtr _number_default_delegate;
	static const SQRegFunction _number_default_delegate_funcz[];
	SQObjectPtr _generator_default_delegate;
	static const SQRegFunction _generator_default_delegate_funcz[];
	SQObjectPtr _closure_default_delegate;
	static const SQRegFunction _closure_default_delegate_funcz[];
	SQObjectPtr _thread_default_delegate;
	static const SQRegFunction _thread_default_delegate_funcz[];
	SQObjectPtr _class_default_delegate;
	static const SQRegFunction _class_default_delegate_funcz[];
	SQObjectPtr _instance_default_delegate;
	static const SQRegFunction _instance_default_delegate_funcz[];
	SQObjectPtr _weakref_default_delegate;
	static const SQRegFunction _weakref_default_delegate_funcz[];

	SQCOMPILERERROR _compilererrorhandler;
	SQLOADUNIT _unitloaderhandler;
	SQPRINTFUNCTION _printfunc;
	SQPRINTFUNCTION _errorfunc;
	bool _debuginfo;
	bool _notifyallexceptions;
	LVUserPointer _foreignptr;
	SQRELEASEHOOK _releasehook;

  private:
	LVChar *_scratchpad;
	LVInteger _scratchpadsize;
};

#define _sp(s) (_sharedstate->GetScratchPad(s))
#define _spval (_sharedstate->GetScratchPad(-1))

#define _table_ddel     _table(_sharedstate->_table_default_delegate)
#define _array_ddel     _table(_sharedstate->_array_default_delegate)
#define _string_ddel    _table(_sharedstate->_string_default_delegate)
#define _number_ddel    _table(_sharedstate->_number_default_delegate)
#define _generator_ddel _table(_sharedstate->_generator_default_delegate)
#define _closure_ddel   _table(_sharedstate->_closure_default_delegate)
#define _thread_ddel    _table(_sharedstate->_thread_default_delegate)
#define _class_ddel     _table(_sharedstate->_class_default_delegate)
#define _instance_ddel  _table(_sharedstate->_instance_default_delegate)
#define _weakref_ddel   _table(_sharedstate->_weakref_default_delegate)

bool CompileTypemask(SQIntVec& res, const LVChar *typemask);

#endif // _STATE_H_
