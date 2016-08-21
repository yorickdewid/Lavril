#ifndef _STATE_H_
#define _STATE_H_

#include "utils.h"
#include "object.h"

struct LVString;
struct LVTable;

/* Max number of character for a printed number */
#define NUMBER_MAX_CHAR 50

struct LVStringTable {
	LVStringTable(LVSharedState *ss);
	~LVStringTable();
	LVString *Add(const LVChar *, LVInteger len);
	void Remove(LVString *);

  private:
	void Resize(LVInteger size);
	void AllocNodes(LVInteger size);
	LVString **_strings;
	LVUnsignedInteger _numofslots;
	LVUnsignedInteger _slotused;
	LVSharedState *_sharedstate;
};

struct RefTable {
	struct RefNode {
		LVObjectPtr obj;
		LVUnsignedInteger refs;
		struct RefNode *next;
	};
	RefTable();
	~RefTable();
	void AddRef(LVObject& obj);
	LVBool Release(LVObject& obj);
	LVUnsignedInteger GetRefCount(LVObject& obj);
#ifndef NO_GARBAGE_COLLECTOR
	void Mark(LVCollectable **chain);
#endif
	void Finalize();
  private:
	RefNode *Get(LVObject& obj, LVHash& mainpos, RefNode **prev, bool add);
	RefNode *Add(LVHash mainpos, LVObject& obj);
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

struct LVObjectPtr;

struct LVSharedState {
	LVSharedState();
	~LVSharedState();
	void Init();

  public:
	LVChar *GetScratchPad(LVInteger size);
	LVInteger GetMetaMethodIdxByName(const LVObjectPtr& name);
#ifndef NO_GARBAGE_COLLECTOR
	LVInteger CollectGarbage(LVVM *vm);
	void RunMark(LVVM *vm, LVCollectable **tchain);
	LVInteger ResurrectUnreachable(LVVM *vm);
	static void MarkObject(LVObjectPtr& o, LVCollectable **chain);
#endif
	LVObjectPtrVec *_metamethods;
	LVObjectPtr _metamethodsmap;
	LVObjectPtrVec *_systemstrings;
	LVObjectPtrVec *_types;
	LVStringTable *_stringtable;
	RefTable _refs_table;
	LVObjectPtr _registry;
	LVObjectPtr _consts;
	LVObjectPtr _constructoridx;
#ifndef NO_GARBAGE_COLLECTOR
	LVCollectable *_gc_chain;
#endif
	LVObjectPtr _root_vm;
	LVObjectPtr _table_default_delegate;
	static const LVRegFunction _table_default_delegate_funcz[];
	LVObjectPtr _array_default_delegate;
	static const LVRegFunction _array_default_delegate_funcz[];
	LVObjectPtr _string_default_delegate;
	static const LVRegFunction _string_default_delegate_funcz[];
	LVObjectPtr _number_default_delegate;
	static const LVRegFunction _number_default_delegate_funcz[];
	LVObjectPtr _generator_default_delegate;
	static const LVRegFunction _generator_default_delegate_funcz[];
	LVObjectPtr _closure_default_delegate;
	static const LVRegFunction _closure_default_delegate_funcz[];
	LVObjectPtr _thread_default_delegate;
	static const LVRegFunction _thread_default_delegate_funcz[];
	LVObjectPtr _class_default_delegate;
	static const LVRegFunction _class_default_delegate_funcz[];
	LVObjectPtr _instance_default_delegate;
	static const LVRegFunction _instance_default_delegate_funcz[];
	LVObjectPtr _weakref_default_delegate;
	static const LVRegFunction _weakref_default_delegate_funcz[];

	LVCOMPILERERROR _compilererrorhandler;
	LVPRINTFUNCTION _printfunc;
	LVREADFUNCTION _readfunc;
	LVPRINTFUNCTION _errorfunc;
	bool _debuginfo;
	bool _notifyallexceptions;
	LVUserPointer _foreignptr;
	LVRELEASEHOOK _releasehook;

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

bool CompileTypemask(LVIntVector& res, const LVChar *typemask);

#endif // _STATE_H_
