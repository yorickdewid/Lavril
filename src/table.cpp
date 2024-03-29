#include "pcheader.h"
#include "vm.h"
#include "table.h"
#include "funcproto.h"
#include "closure.h"

LVTable::LVTable(LVSharedState *ss, LVInteger nInitialSize) {
	LVInteger pow2size = MINPOWER2;
	while (nInitialSize > pow2size)pow2size = pow2size << 1;
	AllocNodes(pow2size);
	_usednodes = 0;
	_delegate = NULL;
	INIT_CHAIN();
	ADD_TO_CHAIN(&_sharedstate->_gc_chain, this);
}

void LVTable::Remove(const LVObjectPtr& key) {
	_HashNode *n = _Get(key, HashObj(key) & (_numofnodes - 1));
	if (n) {
		n->val.Null();
		n->key.Null();
		_usednodes--;
		Rehash(false);
	}
}

void LVTable::AllocNodes(LVInteger nSize) {
	_HashNode *nodes = (_HashNode *)LV_MALLOC(sizeof(_HashNode) * nSize);
	for (LVInteger i = 0; i < nSize; i++) {
		_HashNode& n = nodes[i];
		new (&n) _HashNode;
		n.next = NULL;
	}
	_numofnodes = nSize;
	_nodes = nodes;
	_firstfree = &_nodes[_numofnodes - 1];
}

void LVTable::Rehash(bool force) {
	LVInteger oldsize = _numofnodes;
	//prevent problems with the integer division
	if (oldsize < 4)oldsize = 4;
	_HashNode *nold = _nodes;
	LVInteger nelems = CountUsed();
	if (nelems >= oldsize - oldsize / 4) /* using more than 3/4? */
		AllocNodes(oldsize * 2);
	else if (nelems <= oldsize / 4 && /* less than 1/4? */
	         oldsize > MINPOWER2)
		AllocNodes(oldsize / 2);
	else if (force)
		AllocNodes(oldsize);
	else
		return;
	_usednodes = 0;
	for (LVInteger i = 0; i < oldsize; i++) {
		_HashNode *old = nold + i;
		if (type(old->key) != OT_NULL)
			NewSlot(old->key, old->val);
	}
	for (LVInteger k = 0; k < oldsize; k++)
		nold[k].~_HashNode();
	LV_FREE(nold, oldsize * sizeof(_HashNode));
}

LVTable *LVTable::Clone() {
	LVTable *nt = Create(_opt_ss(this), _numofnodes);
#ifdef _FAST_CLONE
	_HashNode *basesrc = _nodes;
	_HashNode *basedst = nt->_nodes;
	_HashNode *src = _nodes;
	_HashNode *dst = nt->_nodes;
	LVInteger n = 0;
	for (n = 0; n < _numofnodes; n++) {
		dst->key = src->key;
		dst->val = src->val;
		if (src->next) {
			assert(src->next > basesrc);
			dst->next = basedst + (src->next - basesrc);
			assert(dst != dst->next);
		}
		dst++;
		src++;
	}
	assert(_firstfree > basesrc);
	assert(_firstfree != NULL);
	nt->_firstfree = basedst + (_firstfree - basesrc);
	nt->_usednodes = _usednodes;
#else
	LVInteger ridx = 0;
	LVObjectPtr key, val;
	while ((ridx = Next(true, ridx, key, val)) != -1) {
		nt->NewSlot(key, val);
	}
#endif
	nt->SetDelegate(_delegate);
	return nt;
}

bool LVTable::Get(const LVObjectPtr& key, LVObjectPtr& val) {
	if (type(key) == OT_NULL)
		return false;
	_HashNode *n = _Get(key, HashObj(key) & (_numofnodes - 1));
	if (n) {
		val = _realval(n->val);
		return true;
	}
	return false;
}

bool LVTable::NewSlot(const LVObjectPtr& key, const LVObjectPtr& val) {
	assert(type(key) != OT_NULL);
	LVHash h = HashObj(key) & (_numofnodes - 1);
	_HashNode *n = _Get(key, h);
	if (n) {
		n->val = val;
		return false;
	}
	_HashNode *mp = &_nodes[h];
	n = mp;


	//key not found I'll insert it
	//main pos is not free

	if (type(mp->key) != OT_NULL) {
		n = _firstfree;  /* get a free place */
		LVHash mph = HashObj(mp->key) & (_numofnodes - 1);
		_HashNode *othern;  /* main position of colliding node */

		if (mp > n && (othern = &_nodes[mph]) != mp) {
			/* yes; move colliding node into free position */
			while (othern->next != mp) {
				assert(othern->next != NULL);
				othern = othern->next;  /* find previous */
			}
			othern->next = n;  /* redo the chain with `n' in place of `mp' */
			n->key = mp->key;
			n->val = mp->val;/* copy colliding node into free pos. (mp->next also goes) */
			n->next = mp->next;
			mp->key.Null();
			mp->val.Null();
			mp->next = NULL;  /* now `mp' is free */
		} else {
			/* new node will go into free position */
			n->next = mp->next;  /* chain new position */
			mp->next = n;
			mp = n;
		}
	}
	mp->key = key;

	for (;;) {  /* correct `firstfree' */
		if (type(_firstfree->key) == OT_NULL && _firstfree->next == NULL) {
			mp->val = val;
			_usednodes++;
			return true;  /* OK; table still has a free place */
		} else if (_firstfree == _nodes) break; /* cannot decrement from here */
		else (_firstfree)--;
	}
	Rehash(true);
	return NewSlot(key, val);
}

LVInteger LVTable::Next(bool getweakrefs, const LVObjectPtr& refpos, LVObjectPtr& outkey, LVObjectPtr& outval) {
	LVInteger idx = (LVInteger)TranslateIndex(refpos);
	while (idx < _numofnodes) {
		if (type(_nodes[idx].key) != OT_NULL) {
			//first found
			_HashNode& n = _nodes[idx];
			outkey = n.key;
			outval = getweakrefs ? (LVObject)n.val : _realval(n.val);
			//return idx for the next iteration
			return ++idx;
		}
		++idx;
	}
	//nothing to iterate anymore
	return -1;
}


bool LVTable::Set(const LVObjectPtr& key, const LVObjectPtr& val) {
	_HashNode *n = _Get(key, HashObj(key) & (_numofnodes - 1));
	if (n) {
		n->val = val;
		return true;
	}
	return false;
}

void LVTable::_ClearNodes() {
	for (LVInteger i = 0; i < _numofnodes; i++) {
		_HashNode& n = _nodes[i];
		n.key.Null();
		n.val.Null();
	}
}

void LVTable::Finalize() {
	_ClearNodes();
	SetDelegate(NULL);
}

void LVTable::Clear() {
	_ClearNodes();
	_usednodes = 0;
	Rehash(true);
}
