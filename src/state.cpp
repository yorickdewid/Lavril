#include "pcheader.h"
#include "opcodes.h"
#include "vm.h"
#include "funcproto.h"
#include "closure.h"
#include "lvstring.h"
#include "table.h"
#include "array.h"
#include "userdata.h"
#include "class.h"

LVSharedState::LVSharedState() {
	_compilererrorhandler = NULL;
	_printfunc = NULL;
	_errorfunc = NULL;
	_debuginfo = false;
	_notifyallexceptions = false;
	_foreignptr = NULL;
	_releasehook = NULL;
}

#define newsysstring(s) {   \
    _systemstrings->push_back(LVString::Create(this,s));    \
    }

#define newmetamethod(s) {  \
    _metamethods->push_back(LVString::Create(this,s));  \
    _table(_metamethodsmap)->NewSlot(_metamethods->back(),(LVInteger)(_metamethods->size()-1)); \
    }

bool CompileTypemask(LVIntVector& res, const LVChar *typemask) {
	LVInteger i = 0;

	LVInteger mask = 0;
	while (typemask[i] != 0) {
		switch (typemask[i]) {
			case 'o':
				mask |= _RT_NULL;
				break;
			case 'i':
				mask |= _RT_INTEGER;
				break;
			case 'f':
				mask |= _RT_FLOAT;
				break;
			case 'n':
				mask |= (_RT_FLOAT | _RT_INTEGER);
				break;
			case 's':
				mask |= _RT_STRING;
				break;
			case 't':
				mask |= _RT_TABLE;
				break;
			case 'a':
				mask |= _RT_ARRAY;
				break;
			case 'u':
				mask |= _RT_USERDATA;
				break;
			case 'c':
				mask |= (_RT_CLOSURE | _RT_NATIVECLOSURE);
				break;
			case 'b':
				mask |= _RT_BOOL;
				break;
			case 'g':
				mask |= _RT_GENERATOR;
				break;
			case 'p':
				mask |= _RT_USERPOINTER;
				break;
			case 'v':
				mask |= _RT_THREAD;
				break;
			case 'x':
				mask |= _RT_INSTANCE;
				break;
			case 'y':
				mask |= _RT_CLASS;
				break;
			case 'r':
				mask |= _RT_WEAKREF;
				break;
			case '.':
				mask = -1;
				res.push_back(mask);
				i++;
				mask = 0;
				continue;
			case ' ':
				i++;
				continue; //ignores spaces
			default:
				return false;
		}
		i++;
		if (typemask[i] == '|') {
			i++;
			if (typemask[i] == 0)
				return false;
			continue;
		}
		res.push_back(mask);
		mask = 0;

	}
	return true;
}

LVTable *CreateDefaultDelegate(LVSharedState *ss, const LVRegFunction *funcz) {
	LVInteger i = 0;
	LVTable *t = LVTable::Create(ss, 0);
	while (funcz[i].name != 0) {
		LVNativeClosure *nc = LVNativeClosure::Create(ss, funcz[i].f, 0);
		nc->_nparamscheck = funcz[i].nparamscheck;
		nc->_name = LVString::Create(ss, funcz[i].name);
		if (funcz[i].typemask && !CompileTypemask(nc->_typecheck, funcz[i].typemask))
			return NULL;
		t->NewSlot(LVString::Create(ss, funcz[i].name), nc);
		i++;
	}
	return t;
}

void LVSharedState::Init() {
	_scratchpad = NULL;
	_scratchpadsize = 0;
#ifndef NO_GARBAGE_COLLECTOR
	_gc_chain = NULL;
#endif
	_stringtable = (LVStringTable *)LV_MALLOC(sizeof(LVStringTable));
	new(_stringtable) LVStringTable(this);
	lv_new(_metamethods, LVObjectPtrVec);
	lv_new(_systemstrings, LVObjectPtrVec);
	lv_new(_types, LVObjectPtrVec);
	_metamethodsmap = LVTable::Create(this, MT_LAST - 1);

	//types names
	newsysstring(_LC("null"));
	newsysstring(_LC("table"));
	newsysstring(_LC("array"));
	newsysstring(_LC("closure"));
	newsysstring(_LC("string"));
	newsysstring(_LC("userdata"));
	newsysstring(_LC("integer"));
	newsysstring(_LC("float"));
	newsysstring(_LC("userpointer"));
	newsysstring(_LC("function"));
	newsysstring(_LC("generator"));
	newsysstring(_LC("thread"));
	newsysstring(_LC("class"));
	newsysstring(_LC("instance"));
	newsysstring(_LC("bool"));

	//meta methods
	newmetamethod(MM_ADD);
	newmetamethod(MM_SUB);
	newmetamethod(MM_MUL);
	newmetamethod(MM_DIV);
	newmetamethod(MM_UNM);
	newmetamethod(MM_MODULO);
	newmetamethod(MM_SET);
	newmetamethod(MM_GET);
	newmetamethod(MM_TYPEOF);
	newmetamethod(MM_NEXTI);
	newmetamethod(MM_CMP);
	newmetamethod(MM_CALL);
	newmetamethod(MM_CLONED);
	newmetamethod(MM_NEWSLOT);
	newmetamethod(MM_DELSLOT);
	newmetamethod(MM_TOSTRING);
	newmetamethod(MM_NEWMEMBER);
	newmetamethod(MM_INHERITED);

	_constructoridx = LVString::Create(this, _LC("constructor"));
	_registry = LVTable::Create(this, 0);
	_consts = LVTable::Create(this, 0);
	_table_default_delegate = CreateDefaultDelegate(this, _table_default_delegate_funcz);
	_array_default_delegate = CreateDefaultDelegate(this, _array_default_delegate_funcz);
	_string_default_delegate = CreateDefaultDelegate(this, _string_default_delegate_funcz);
	_number_default_delegate = CreateDefaultDelegate(this, _number_default_delegate_funcz);
	_closure_default_delegate = CreateDefaultDelegate(this, _closure_default_delegate_funcz);
	_generator_default_delegate = CreateDefaultDelegate(this, _generator_default_delegate_funcz);
	_thread_default_delegate = CreateDefaultDelegate(this, _thread_default_delegate_funcz);
	_class_default_delegate = CreateDefaultDelegate(this, _class_default_delegate_funcz);
	_instance_default_delegate = CreateDefaultDelegate(this, _instance_default_delegate_funcz);
	_weakref_default_delegate = CreateDefaultDelegate(this, _weakref_default_delegate_funcz);
}

LVSharedState::~LVSharedState() {
	if (_releasehook) {
		_releasehook(_foreignptr, 0);
		_releasehook = NULL;
	}
	_constructoridx.Null();
	_table(_registry)->Finalize();
	_table(_consts)->Finalize();
	_table(_metamethodsmap)->Finalize();
	_registry.Null();
	_consts.Null();
	_metamethodsmap.Null();
	while (!_systemstrings->empty()) {
		_systemstrings->back().Null();
		_systemstrings->pop_back();
	}
	_thread(_root_vm)->Finalize();
	_root_vm.Null();
	_table_default_delegate.Null();
	_array_default_delegate.Null();
	_string_default_delegate.Null();
	_number_default_delegate.Null();
	_closure_default_delegate.Null();
	_generator_default_delegate.Null();
	_thread_default_delegate.Null();
	_class_default_delegate.Null();
	_instance_default_delegate.Null();
	_weakref_default_delegate.Null();
	_refs_table.Finalize();
#ifndef NO_GARBAGE_COLLECTOR
	LVCollectable *t = _gc_chain;
	LVCollectable *nx = NULL;
	if (t) {
		t->_uiRef++;
		while (t) {
			t->Finalize();
			nx = t->_next;
			if (nx) nx->_uiRef++;
			if (--t->_uiRef == 0)
				t->Release();
			t = nx;
		}
	}
	assert(_gc_chain == NULL); //just to proove a theory
	while (_gc_chain) {
		_gc_chain->_uiRef++;
		_gc_chain->Release();
	}
#endif

	lv_delete(_types, LVObjectPtrVec);
	lv_delete(_systemstrings, LVObjectPtrVec);
	lv_delete(_metamethods, LVObjectPtrVec);
	lv_delete(_stringtable, LVStringTable);
	if (_scratchpad)LV_FREE(_scratchpad, _scratchpadsize);
}


LVInteger LVSharedState::GetMetaMethodIdxByName(const LVObjectPtr& name) {
	if (type(name) != OT_STRING)
		return -1;
	LVObjectPtr ret;
	if (_table(_metamethodsmap)->Get(name, ret)) {
		return _integer(ret);
	}
	return -1;
}

#ifndef NO_GARBAGE_COLLECTOR

void LVSharedState::MarkObject(LVObjectPtr& o, LVCollectable **chain) {
	switch (type(o)) {
		case OT_TABLE:
			_table(o)->Mark(chain);
			break;
		case OT_ARRAY:
			_array(o)->Mark(chain);
			break;
		case OT_USERDATA:
			_userdata(o)->Mark(chain);
			break;
		case OT_CLOSURE:
			_closure(o)->Mark(chain);
			break;
		case OT_NATIVECLOSURE:
			_nativeclosure(o)->Mark(chain);
			break;
		case OT_GENERATOR:
			_generator(o)->Mark(chain);
			break;
		case OT_THREAD:
			_thread(o)->Mark(chain);
			break;
		case OT_CLASS:
			_class(o)->Mark(chain);
			break;
		case OT_INSTANCE:
			_instance(o)->Mark(chain);
			break;
		case OT_OUTER:
			_outer(o)->Mark(chain);
			break;
		case OT_FUNCPROTO:
			_funcproto(o)->Mark(chain);
			break;
		default:
			break; //shutup compiler
	}
}

void LVSharedState::RunMark(LVVM LV_UNUSED_ARG(*vm), LVCollectable **tchain) {
	LVVM *vms = _thread(_root_vm);

	vms->Mark(tchain);

	_refs_table.Mark(tchain);
	MarkObject(_registry, tchain);
	MarkObject(_consts, tchain);
	MarkObject(_metamethodsmap, tchain);
	MarkObject(_table_default_delegate, tchain);
	MarkObject(_array_default_delegate, tchain);
	MarkObject(_string_default_delegate, tchain);
	MarkObject(_number_default_delegate, tchain);
	MarkObject(_generator_default_delegate, tchain);
	MarkObject(_thread_default_delegate, tchain);
	MarkObject(_closure_default_delegate, tchain);
	MarkObject(_class_default_delegate, tchain);
	MarkObject(_instance_default_delegate, tchain);
	MarkObject(_weakref_default_delegate, tchain);
}

LVInteger LVSharedState::ResurrectUnreachable(LVVM *vm) {
	LVInteger n = 0;
	LVCollectable *tchain = NULL;

	RunMark(vm, &tchain);

	LVCollectable *resurrected = _gc_chain;
	LVCollectable *t = resurrected;
	//LVCollectable *nx = NULL;

	_gc_chain = tchain;

	LVArray *ret = NULL;
	if (resurrected) {
		ret = LVArray::Create(this, 0);
		LVCollectable *rlast = NULL;
		while (t) {
			rlast = t;
			LVObjectType type = t->GetType();
			if (type != OT_FUNCPROTO && type != OT_OUTER) {
				LVObject obj;
				obj._type = type;
				obj._unVal.pRefCounted = t;
				ret->Append(obj);
			}
			t = t->_next;
			n++;
		}

		assert(rlast->_next == NULL);
		rlast->_next = _gc_chain;
		if (_gc_chain) {
			_gc_chain->_prev = rlast;
		}
		_gc_chain = resurrected;
	}

	t = _gc_chain;
	while (t) {
		t->UnMark();
		t = t->_next;
	}

	if (ret) {
		LVObjectPtr temp = ret;
		vm->Push(temp);
	} else {
		vm->PushNull();
	}
	return n;
}

LVInteger LVSharedState::CollectGarbage(LVVM *vm) {
	LVInteger n = 0;
	LVCollectable *tchain = NULL;

	RunMark(vm, &tchain);

	LVCollectable *t = _gc_chain;
	LVCollectable *nx = NULL;
	if (t) {
		t->_uiRef++;
		while (t) {
			t->Finalize();
			nx = t->_next;
			if (nx) nx->_uiRef++;
			if (--t->_uiRef == 0)
				t->Release();
			t = nx;
			n++;
		}
	}

	t = tchain;
	while (t) {
		t->UnMark();
		t = t->_next;
	}
	_gc_chain = tchain;

	return n;
}
#endif

#ifndef NO_GARBAGE_COLLECTOR
void LVCollectable::AddToChain(LVCollectable **chain, LVCollectable *c) {
	c->_prev = NULL;
	c->_next = *chain;
	if (*chain) (*chain)->_prev = c;
	*chain = c;
}

void LVCollectable::RemoveFromChain(LVCollectable **chain, LVCollectable *c) {
	if (c->_prev) c->_prev->_next = c->_next;
	else *chain = c->_next;
	if (c->_next)
		c->_next->_prev = c->_prev;
	c->_next = NULL;
	c->_prev = NULL;
}
#endif

LVChar *LVSharedState::GetScratchPad(LVInteger size) {
	LVInteger newsize;
	if (size > 0) {
		if (_scratchpadsize < size) {
			newsize = size + (size >> 1);
			_scratchpad = (LVChar *)LV_REALLOC(_scratchpad, _scratchpadsize, newsize);
			_scratchpadsize = newsize;

		} else if (_scratchpadsize >= (size << 5)) {
			newsize = _scratchpadsize >> 1;
			_scratchpad = (LVChar *)LV_REALLOC(_scratchpad, _scratchpadsize, newsize);
			_scratchpadsize = newsize;
		}
	}
	return _scratchpad;
}

RefTable::RefTable() {
	AllocNodes(4);
}

void RefTable::Finalize() {
	RefNode *nodes = _nodes;
	for (LVUnsignedInteger n = 0; n < _numofslots; n++) {
		nodes->obj.Null();
		nodes++;
	}
}

RefTable::~RefTable() {
	LV_FREE(_buckets, (_numofslots * sizeof(RefNode *)) + (_numofslots * sizeof(RefNode)));
}

#ifndef NO_GARBAGE_COLLECTOR
void RefTable::Mark(LVCollectable **chain) {
	RefNode *nodes = (RefNode *)_nodes;
	for (LVUnsignedInteger n = 0; n < _numofslots; n++) {
		if (type(nodes->obj) != OT_NULL) {
			LVSharedState::MarkObject(nodes->obj, chain);
		}
		nodes++;
	}
}
#endif

void RefTable::AddRef(LVObject& obj) {
	LVHash mainpos;
	RefNode *prev;
	RefNode *ref = Get(obj, mainpos, &prev, true);
	ref->refs++;
}

LVUnsignedInteger RefTable::GetRefCount(LVObject& obj) {
	LVHash mainpos;
	RefNode *prev;
	RefNode *ref = Get(obj, mainpos, &prev, true);
	return ref->refs;
}

LVBool RefTable::Release(LVObject& obj) {
	LVHash mainpos;
	RefNode *prev;
	RefNode *ref = Get(obj, mainpos, &prev, false);
	if (ref) {
		if (--ref->refs == 0) {
			LVObjectPtr o = ref->obj;
			if (prev) {
				prev->next = ref->next;
			} else {
				_buckets[mainpos] = ref->next;
			}
			ref->next = _freelist;
			_freelist = ref;
			_slotused--;
			ref->obj.Null();
			//<<FIXME>>test for shrink?
			return LVTrue;
		}
	} else {
		assert(0);
	}
	return LVFalse;
}

void RefTable::Resize(LVUnsignedInteger size) {
	RefNode **oldbucks = _buckets;
	RefNode *t = _nodes;
	LVUnsignedInteger oldnumofslots = _numofslots;
	AllocNodes(size);
	//rehash
	LVUnsignedInteger nfound = 0;
	for (LVUnsignedInteger n = 0; n < oldnumofslots; n++) {
		if (type(t->obj) != OT_NULL) {
			//add back;
			assert(t->refs != 0);
			RefNode *nn = Add(::HashObj(t->obj) & (_numofslots - 1), t->obj);
			nn->refs = t->refs;
			t->obj.Null();
			nfound++;
		}
		t++;
	}
	assert(nfound == oldnumofslots);
	LV_FREE(oldbucks, (oldnumofslots * sizeof(RefNode *)) + (oldnumofslots * sizeof(RefNode)));
}

RefTable::RefNode *RefTable::Add(LVHash mainpos, LVObject& obj) {
	RefNode *t = _buckets[mainpos];
	RefNode *newnode = _freelist;
	newnode->obj = obj;
	_buckets[mainpos] = newnode;
	_freelist = _freelist->next;
	newnode->next = t;
	assert(newnode->refs == 0);
	_slotused++;
	return newnode;
}

RefTable::RefNode *RefTable::Get(LVObject& obj, LVHash& mainpos, RefNode **prev, bool add) {
	RefNode *ref;
	mainpos = ::HashObj(obj) & (_numofslots - 1);
	*prev = NULL;
	for (ref = _buckets[mainpos]; ref; ) {
		if (_rawval(ref->obj) == _rawval(obj) && type(ref->obj) == type(obj))
			break;
		*prev = ref;
		ref = ref->next;
	}
	if (ref == NULL && add) {
		if (_numofslots == _slotused) {
			assert(_freelist == 0);
			Resize(_numofslots * 2);
			mainpos = ::HashObj(obj) & (_numofslots - 1);
		}
		ref = Add(mainpos, obj);
	}
	return ref;
}

void RefTable::AllocNodes(LVUnsignedInteger size) {
	RefNode **bucks;
	RefNode *nodes;
	bucks = (RefNode **)LV_MALLOC((size * sizeof(RefNode *)) + (size * sizeof(RefNode)));
	nodes = (RefNode *)&bucks[size];
	RefNode *temp = nodes;
	LVUnsignedInteger n;
	for (n = 0; n < size - 1; n++) {
		bucks[n] = NULL;
		temp->refs = 0;
		new (&temp->obj) LVObjectPtr;
		temp->next = temp + 1;
		temp++;
	}
	bucks[n] = NULL;
	temp->refs = 0;
	new (&temp->obj) LVObjectPtr;
	temp->next = NULL;
	_freelist = nodes;
	_nodes = nodes;
	_buckets = bucks;
	_slotused = 0;
	_numofslots = size;
}
//////////////////////////////////////////////////////////////////////////
//LVStringTable
/*
* The following code is based on Lua 4.0 (Copyright 1994-2002 Tecgraf, PUC-Rio.)
* http://www.lua.org/copyright.html#4
* http://www.lua.org/source/4.0.1/src_lstring.c.html
*/

LVStringTable::LVStringTable(LVSharedState *ss) {
	_sharedstate = ss;
	AllocNodes(4);
	_slotused = 0;
}

LVStringTable::~LVStringTable() {
	LV_FREE(_strings, sizeof(LVString *)*_numofslots);
	_strings = NULL;
}

void LVStringTable::AllocNodes(LVInteger size) {
	_numofslots = size;
	_strings = (LVString **)LV_MALLOC(sizeof(LVString *)*_numofslots);
	memset(_strings, 0, sizeof(LVString *)*_numofslots);
}

LVString *LVStringTable::Add(const LVChar *news, LVInteger len) {
	if (len < 0)
		len = (LVInteger)scstrlen(news);
	LVHash newhash = ::_hashstr(news, len);
	LVHash h = newhash & (_numofslots - 1);
	LVString *s;
	for (s = _strings[h]; s; s = s->_next) {
		if (s->_len == len && (!memcmp(news, s->_val, lv_rsl(len))))
			return s; //found
	}

	LVString *t = (LVString *)LV_MALLOC(lv_rsl(len) + sizeof(LVString));
	new (t) LVString;
	t->_sharedstate = _sharedstate;
	memcpy(t->_val, news, lv_rsl(len));
	t->_val[len] = _LC('\0');
	t->_len = len;
	t->_hash = newhash;
	t->_next = _strings[h];
	_strings[h] = t;
	_slotused++;
	if (_slotused > _numofslots)  /* too crowded? */
		Resize(_numofslots * 2);
	return t;
}

void LVStringTable::Resize(LVInteger size) {
	LVInteger oldsize = _numofslots;
	LVString **oldtable = _strings;
	AllocNodes(size);
	for (LVInteger i = 0; i < oldsize; i++) {
		LVString *p = oldtable[i];
		while (p) {
			LVString *next = p->_next;
			LVHash h = p->_hash & (_numofslots - 1);
			p->_next = _strings[h];
			_strings[h] = p;
			p = next;
		}
	}
	LV_FREE(oldtable, oldsize * sizeof(LVString *));
}

void LVStringTable::Remove(LVString *bs) {
	LVString *s;
	LVString *prev = NULL;
	LVHash h = bs->_hash & (_numofslots - 1);

	for (s = _strings[h]; s; ) {
		if (s == bs) {
			if (prev)
				prev->_next = s->_next;
			else
				_strings[h] = s->_next;
			_slotused--;
			LVInteger slen = s->_len;
			s->~LVString();
			LV_FREE(s, sizeof(LVString) + lv_rsl(slen));
			return;
		}
		prev = s;
		s = s->_next;
	}
	assert(0);//if this fail something is wrong
}
