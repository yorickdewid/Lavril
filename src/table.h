#ifndef _TABLE_H_
#define _TABLE_H_

#include "lvstring.h"

#define hashptr(p)  ((LVHash)(((LVInteger)p) >> 3))

inline LVHash HashObj(const SQObjectPtr& key) {
	switch (type(key)) {
		case OT_STRING:
			return _string(key)->_hash;
		case OT_FLOAT:
			return (LVHash)((LVInteger)_float(key));
		case OT_BOOL:
		case OT_INTEGER:
			return (LVHash)((LVInteger)_integer(key));
		default:
			return hashptr(key._unVal.pRefCounted);
	}
}

struct SQTable : public SQDelegable {
  private:
	struct _HashNode {
		_HashNode() {
			next = NULL;
		}
		SQObjectPtr val;
		SQObjectPtr key;
		_HashNode *next;
	};

	_HashNode *_firstfree;
	_HashNode *_nodes;
	LVInteger _numofnodes;
	LVInteger _usednodes;

	///////////////////////////
	void AllocNodes(LVInteger nSize);
	void Rehash(bool force);
	SQTable(SQSharedState *ss, LVInteger nInitialSize);
	void _ClearNodes();

  public:
	static SQTable *Create(SQSharedState *ss, LVInteger nInitialSize) {
		SQTable *newtable = (SQTable *)LV_MALLOC(sizeof(SQTable));
		new (newtable) SQTable(ss, nInitialSize);
		newtable->_delegate = NULL;
		return newtable;
	}
	void Finalize();
	SQTable *Clone();
	~SQTable() {
		SetDelegate(NULL);
		REMOVE_FROM_CHAIN(&_sharedstate->_gc_chain, this);
		for (LVInteger i = 0; i < _numofnodes; i++)
			_nodes[i].~_HashNode();
		LV_FREE(_nodes, _numofnodes * sizeof(_HashNode));
	}
#ifndef NO_GARBAGE_COLLECTOR
	void Mark(SQCollectable **chain);
	SQObjectType GetType() {
		return OT_TABLE;
	}
#endif
	inline _HashNode *_Get(const SQObjectPtr& key, LVHash hash) {
		_HashNode *n = &_nodes[hash];
		do {
			if (_rawval(n->key) == _rawval(key) && type(n->key) == type(key)) {
				return n;
			}
		} while ((n = n->next));
		return NULL;
	}
	//for compiler use
	inline bool GetStr(const LVChar *key, LVInteger keylen, SQObjectPtr& val) {
		LVHash hash = _hashstr(key, keylen);
		_HashNode *n = &_nodes[hash & (_numofnodes - 1)];
		_HashNode *res = NULL;
		do {
			if (type(n->key) == OT_STRING && (scstrcmp(_stringval(n->key), key) == 0)) {
				res = n;
				break;
			}
		} while ((n = n->next));
		if (res) {
			val = _realval(res->val);
			return true;
		}
		return false;
	}
	bool Get(const SQObjectPtr& key, SQObjectPtr& val);
	void Remove(const SQObjectPtr& key);
	bool Set(const SQObjectPtr& key, const SQObjectPtr& val);
	//returns true if a new slot has been created false if it was already present
	bool NewSlot(const SQObjectPtr& key, const SQObjectPtr& val);
	LVInteger Next(bool getweakrefs, const SQObjectPtr& refpos, SQObjectPtr& outkey, SQObjectPtr& outval);

	LVInteger CountUsed() {
		return _usednodes;
	}
	void Clear();
	void Release() {
		sq_delete(this, SQTable);
	}

};

#endif // _TABLE_H_
