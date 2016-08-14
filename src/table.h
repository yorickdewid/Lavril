#ifndef _TABLE_H_
#define _TABLE_H_

#include "lvstring.h"

#define hashptr(p)  ((LVHash)(((LVInteger)p) >> 3))

inline LVHash HashObj(const LVObjectPtr& key) {
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

struct LVTable : public LVDelegable {
  private:
	struct _HashNode {
		_HashNode() {
			next = NULL;
		}
		LVObjectPtr val;
		LVObjectPtr key;
		_HashNode *next;
	};

	_HashNode *_firstfree;
	_HashNode *_nodes;
	LVInteger _numofnodes;
	LVInteger _usednodes;

	///////////////////////////
	void AllocNodes(LVInteger nSize);
	void Rehash(bool force);
	LVTable(LVSharedState *ss, LVInteger nInitialSize);
	void _ClearNodes();

  public:
	static LVTable *Create(LVSharedState *ss, LVInteger nInitialSize) {
		LVTable *newtable = (LVTable *)LV_MALLOC(sizeof(LVTable));
		new (newtable) LVTable(ss, nInitialSize);
		newtable->_delegate = NULL;
		return newtable;
	}
	void Finalize();
	LVTable *Clone();
	~LVTable() {
		SetDelegate(NULL);
		REMOVE_FROM_CHAIN(&_sharedstate->_gc_chain, this);
		for (LVInteger i = 0; i < _numofnodes; i++)
			_nodes[i].~_HashNode();
		LV_FREE(_nodes, _numofnodes * sizeof(_HashNode));
	}
#ifndef NO_GARBAGE_COLLECTOR
	void Mark(LVCollectable **chain);
	LVObjectType GetType() {
		return OT_TABLE;
	}
#endif
	inline _HashNode *_Get(const LVObjectPtr& key, LVHash hash) {
		_HashNode *n = &_nodes[hash];
		do {
			if (_rawval(n->key) == _rawval(key) && type(n->key) == type(key)) {
				return n;
			}
		} while ((n = n->next));
		return NULL;
	}
	//for compiler use
	inline bool GetStr(const LVChar *key, LVInteger keylen, LVObjectPtr& val) {
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
	bool Get(const LVObjectPtr& key, LVObjectPtr& val);
	void Remove(const LVObjectPtr& key);
	bool Set(const LVObjectPtr& key, const LVObjectPtr& val);
	//returns true if a new slot has been created false if it was already present
	bool NewSlot(const LVObjectPtr& key, const LVObjectPtr& val);
	LVInteger Next(bool getweakrefs, const LVObjectPtr& refpos, LVObjectPtr& outkey, LVObjectPtr& outval);

	LVInteger CountUsed() {
		return _usednodes;
	}
	void Clear();
	void Release() {
		lv_delete(this, LVTable);
	}

};

#endif // _TABLE_H_
