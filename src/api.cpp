#include "pcheader.h"
#include "vm.h"
#include "lvstring.h"
#include "table.h"
#include "array.h"
#include "funcproto.h"
#include "closure.h"
#include "userdata.h"
#include "compiler.h"
#include "funcstate.h"
#include "class.h"

static bool aux_gettypedarg(VMHANDLE v, LVInteger idx, LVObjectType type, LVObjectPtr **o) {
	*o = &stack_get(v, idx);
	if (type(**o) != type) {
		LVObjectPtr oval = v->PrintObjVal(**o);
		v->Raise_Error(_LC("wrong argument type, expected '%s' got '%.50s'"), IdType2Name(type), _stringval(oval));
		return false;
	}
	return true;
}

#define _GETSAFE_OBJ(v,idx,type,o) { \
    if(!aux_gettypedarg(v,idx,type,&o)) \
        return LV_ERROR; \
    }

#define aux_paramscheck(v,count) { \
    if(lv_gettop(v) < count) { \
    	v->Raise_Error(_LC("not enough params in the stack")); \
    	return LV_ERROR; \
    }\
}

LVInteger aux_invalidtype(VMHANDLE v, LVObjectType type) {
	LVUnsignedInteger buf_size = 100 * sizeof(LVChar);
	scsprintf(_ss(v)->GetScratchPad(buf_size), buf_size, _LC("unexpected type %s"), IdType2Name(type));
	return lv_throwerror(v, _ss(v)->GetScratchPad(-1));
}

/* Open new VM instance */
VMHANDLE lv_open(LVInteger initialstacksize) {
	LVSharedState *ss = NULL;
	LVVM *v = NULL;
	lv_new(ss, LVSharedState);
	ss->Init();
	v = (LVVM *)LV_MALLOC(sizeof(LVVM));
	new (v) LVVM(ss);
	ss->_root_vm = v;
	if (v->Init(NULL, initialstacksize)) {
		return v;
	} else {
		lv_delete(v, LVVM);
		return NULL;
	}
	return v;
}

VMHANDLE lv_newthread(VMHANDLE friendvm, LVInteger initialstacksize) {
	LVSharedState *ss = NULL;
	LVVM *v = NULL;
	ss = _ss(friendvm);

	v = (LVVM *)LV_MALLOC(sizeof(LVVM));
	new (v) LVVM(ss);

	if (v->Init(friendvm, initialstacksize)) {
		friendvm->Push(v);
		return v;
	} else {
		lv_delete(v, LVVM);
		return NULL;
	}
}

LVInteger lv_getvmstate(VMHANDLE v) {
	if (v->_suspended)
		return LV_VMSTATE_SUSPENDED;
	else {
		if (v->_callsstacksize != 0)
			return LV_VMSTATE_RUNNING;
		else return LV_VMSTATE_IDLE;
	}
}

/* Runtime error callback */
void lv_seterrorhandler(VMHANDLE v) {
	LVObject o = stack_get(v, -1);
	if (lv_isclosure(o) || lv_isnativeclosure(o) || lv_isnull(o)) {
		v->_errorhandler = o;
		v->Pop();
	}
}

void lv_setnativedebughook(VMHANDLE v, LVDEBUGHOOK hook) {
	v->_debughook_native = hook;
	v->_debughook_closure.Null();
	v->_debughook = hook ? true : false;
}

void lv_setdebughook(VMHANDLE v) {
	LVObject o = stack_get(v, -1);
	if (lv_isclosure(o) || lv_isnativeclosure(o) || lv_isnull(o)) {
		v->_debughook_closure = o;
		v->_debughook_native = NULL;
		v->_debughook = !lv_isnull(o);
		v->Pop();
	}
}

/* Release VM instance */
void lv_close(VMHANDLE v) {
	LVSharedState *ss = _ss(v);
	_thread(ss->_root_vm)->Finalize();
	lv_delete(ss, LVSharedState);
}

LVInteger lv_getversion() {
	return LAVRIL_VERSION_NUMBER;
}

/* Compile source */
LVRESULT lv_compile(VMHANDLE v, LVLEXREADFUNC read, LVUserPointer p, const LVChar *sourcename, LVBool raiseerror) {
	LVObjectPtr o;
#ifndef NO_COMPILER
	if (RunCompiler(v, read, p, sourcename, o, raiseerror ? true : false, _ss(v)->_debuginfo)) {
		v->Push(LVClosure::Create(_ss(v), _funcproto(o), _table(v->_roottable)->GetWeakRef(OT_TABLE)));
		return LV_OK;
	}
	return LV_ERROR;
#else
	return lv_throwerror(v, _LC("this is a no compiler build"));
#endif
}

void lv_enabledebuginfo(VMHANDLE v, LVBool enable) {
	_ss(v)->_debuginfo = enable ? true : false;
}

void lv_notifyallexceptions(VMHANDLE v, LVBool enable) {
	_ss(v)->_notifyallexceptions = enable ? true : false;
}

void lv_addref(VMHANDLE v, OBJHANDLE *po) {
	if (!ISREFCOUNTED(type(*po))) return;
#ifdef NO_GARBAGE_COLLECTOR
	__AddRef(po->_type, po->_unVal);
#else
	_ss(v)->_refs_table.AddRef(*po);
#endif
}

LVUnsignedInteger lv_getrefcount(VMHANDLE v, OBJHANDLE *po) {
	if (!ISREFCOUNTED(type(*po))) return 0;
#ifdef NO_GARBAGE_COLLECTOR
	return po->_unVal.pRefCounted->_uiRef;
#else
	return _ss(v)->_refs_table.GetRefCount(*po);
#endif
}

LVBool lv_release(VMHANDLE v, OBJHANDLE *po) {
	if (!ISREFCOUNTED(type(*po)))
		return LVTrue;
#ifdef NO_GARBAGE_COLLECTOR
	bool ret = (po->_unVal.pRefCounted->_uiRef <= 1) ? LVTrue : LVFalse;
	__Release(po->_type, po->_unVal);
	return ret; //the ret val doesn't work(and cannot be fixed)
#else
	return _ss(v)->_refs_table.Release(*po);
#endif
}

LVUnsignedInteger lv_getvmrefcount(VMHANDLE LV_UNUSED_ARG(v), const OBJHANDLE *po) {
	if (!ISREFCOUNTED(type(*po)))
		return 0;
	return po->_unVal.pRefCounted->_uiRef;
}

const LVChar *lv_objtostring(const OBJHANDLE *o) {
	if (lv_type(*o) == OT_STRING) {
		return _stringval(*o);
	}
	return NULL;
}

LVInteger lv_objtointeger(const OBJHANDLE *o) {
	if (lv_isnumeric(*o)) {
		return tointeger(*o);
	}
	return 0;
}

LVFloat lv_objtofloat(const OBJHANDLE *o) {
	if (lv_isnumeric(*o)) {
		return tofloat(*o);
	}
	return 0;
}

LVBool lv_objtobool(const OBJHANDLE *o) {
	if (lv_isbool(*o)) {
		return _integer(*o);
	}
	return LVFalse;
}

LVUserPointer lv_objtouserpointer(const OBJHANDLE *o) {
	if (lv_isuserpointer(*o)) {
		return _userpointer(*o);
	}
	return 0;
}

void lv_pushnull(VMHANDLE v) {
	v->PushNull();
}

void lv_pushstring(VMHANDLE v, const LVChar *s, LVInteger len) {
	if (s)
		v->Push(LVObjectPtr(LVString::Create(_ss(v), s, len)));
	else
		v->PushNull();
}

void lv_pushinteger(VMHANDLE v, LVInteger n) {
	v->Push(n);
}

void lv_pushbool(VMHANDLE v, LVBool b) {
	v->Push(b ? true : false);
}

void lv_pushfloat(VMHANDLE v, LVFloat n) {
	v->Push(n);
}

void lv_pushuserpointer(VMHANDLE v, LVUserPointer p) {
	v->Push(p);
}

void lv_pushthread(VMHANDLE v, VMHANDLE thread) {
	v->Push(thread);
}

LVUserPointer lv_newuserdata(VMHANDLE v, LVUnsignedInteger size) {
	LVUserData *ud = LVUserData::Create(_ss(v), size + LV_ALIGNMENT);
	v->Push(ud);
	return (LVUserPointer)LV_ALIGN(ud + 1);
}

void lv_newtable(VMHANDLE v) {
	v->Push(LVTable::Create(_ss(v), 0));
}

void lv_newtableex(VMHANDLE v, LVInteger initialcapacity) {
	v->Push(LVTable::Create(_ss(v), initialcapacity));
}

void lv_newarray(VMHANDLE v, LVInteger size) {
	v->Push(LVArray::Create(_ss(v), size));
}

LVRESULT lv_newclass(VMHANDLE v, LVBool hasbase) {
	LVClass *baseclass = NULL;
	if (hasbase) {
		LVObjectPtr& base = stack_get(v, -1);
		if (type(base) != OT_CLASS)
			return lv_throwerror(v, _LC("invalid base type"));
		baseclass = _class(base);
	}
	LVClass *newclass = LVClass::Create(_ss(v), baseclass);
	if (baseclass)
		v->Pop();
	v->Push(newclass);
	return LV_OK;
}

LVBool lv_instanceof(VMHANDLE v) {
	LVObjectPtr& inst = stack_get(v, -1);
	LVObjectPtr& cl = stack_get(v, -2);
	if (type(inst) != OT_INSTANCE || type(cl) != OT_CLASS)
		return lv_throwerror(v, _LC("invalid param type"));
	return _instance(inst)->InstanceOf(_class(cl)) ? LVTrue : LVFalse;
}

LVRESULT lv_arrayappend(VMHANDLE v, LVInteger idx) {
	aux_paramscheck(v, 2);
	LVObjectPtr *arr;
	_GETSAFE_OBJ(v, idx, OT_ARRAY, arr);
	_array(*arr)->Append(v->GetUp(-1));
	v->Pop();
	return LV_OK;
}

LVRESULT lv_arraypop(VMHANDLE v, LVInteger idx, LVBool pushval) {
	aux_paramscheck(v, 1);
	LVObjectPtr *arr;
	_GETSAFE_OBJ(v, idx, OT_ARRAY, arr);
	if (_array(*arr)->Size() > 0) {
		if (pushval != 0) {
			v->Push(_array(*arr)->Top());
		}
		_array(*arr)->Pop();
		return LV_OK;
	}
	return lv_throwerror(v, _LC("empty array"));
}

LVRESULT lv_arrayresize(VMHANDLE v, LVInteger idx, LVInteger newsize) {
	aux_paramscheck(v, 1);
	LVObjectPtr *arr;
	_GETSAFE_OBJ(v, idx, OT_ARRAY, arr);
	if (newsize >= 0) {
		_array(*arr)->Resize(newsize);
		return LV_OK;
	}
	return lv_throwerror(v, _LC("negative size"));
}

LVRESULT lv_arrayreverse(VMHANDLE v, LVInteger idx) {
	aux_paramscheck(v, 1);
	LVObjectPtr *o;
	_GETSAFE_OBJ(v, idx, OT_ARRAY, o);
	LVArray *arr = _array(*o);
	if (arr->Size() > 0) {
		LVObjectPtr t;
		LVInteger size = arr->Size();
		LVInteger n = size >> 1;
		size -= 1;
		for (LVInteger i = 0; i < n; i++) {
			t = arr->_values[i];
			arr->_values[i] = arr->_values[size - i];
			arr->_values[size - i] = t;
		}
		return LV_OK;
	}
	return LV_OK;
}

LVRESULT lv_arrayremove(VMHANDLE v, LVInteger idx, LVInteger itemidx) {
	aux_paramscheck(v, 1);
	LVObjectPtr *arr;
	_GETSAFE_OBJ(v, idx, OT_ARRAY, arr);
	return _array(*arr)->Remove(itemidx) ? LV_OK : lv_throwerror(v, _LC("index out of range"));
}

LVRESULT lv_arrayinsert(VMHANDLE v, LVInteger idx, LVInteger destpos) {
	aux_paramscheck(v, 1);
	LVObjectPtr *arr;
	_GETSAFE_OBJ(v, idx, OT_ARRAY, arr);
	LVRESULT ret = _array(*arr)->Insert(destpos, v->GetUp(-1)) ? LV_OK : lv_throwerror(v, _LC("index out of range"));
	v->Pop();
	return ret;
}

void lv_newclosure(VMHANDLE v, LVFUNCTION func, LVUnsignedInteger nfreevars) {
	LVNativeClosure *nc = LVNativeClosure::Create(_ss(v), func, nfreevars);
	nc->_nparamscheck = 0;
	for (LVUnsignedInteger i = 0; i < nfreevars; i++) {
		nc->_outervalues[i] = v->Top();
		v->Pop();
	}
	v->Push(LVObjectPtr(nc));
}

LVRESULT lv_getclosureinfo(VMHANDLE v, LVInteger idx, LVUnsignedInteger *nparams, LVUnsignedInteger *nfreevars) {
	LVObject o = stack_get(v, idx);
	if (type(o) == OT_CLOSURE) {
		LVClosure *c = _closure(o);
		FunctionPrototype *proto = c->_function;
		*nparams = (LVUnsignedInteger)proto->_nparameters;
		*nfreevars = (LVUnsignedInteger)proto->_noutervalues;
		return LV_OK;
	} else if (type(o) == OT_NATIVECLOSURE) {
		LVNativeClosure *c = _nativeclosure(o);
		*nparams = (LVUnsignedInteger)c->_nparamscheck;
		*nfreevars = c->_noutervalues;
		return LV_OK;
	}
	return lv_throwerror(v, _LC("the object is not a closure"));
}

LVRESULT lv_setnativeclosurename(VMHANDLE v, LVInteger idx, const LVChar *name) {
	LVObject o = stack_get(v, idx);
	if (lv_isnativeclosure(o)) {
		LVNativeClosure *nc = _nativeclosure(o);
		nc->_name = LVString::Create(_ss(v), name);
		return LV_OK;
	}
	return lv_throwerror(v, _LC("the object is not a nativeclosure"));
}

LVRESULT lv_setparamscheck(VMHANDLE v, LVInteger nparamscheck, const LVChar *typemask) {
	LVObject o = stack_get(v, -1);
	if (!lv_isnativeclosure(o))
		return lv_throwerror(v, _LC("native closure expected"));
	LVNativeClosure *nc = _nativeclosure(o);
	nc->_nparamscheck = nparamscheck;
	if (typemask) {
		LVIntVector res;
		if (!CompileTypemask(res, typemask))
			return lv_throwerror(v, _LC("invalid typemask"));
		nc->_typecheck.copy(res);
	} else {
		nc->_typecheck.resize(0);
	}
	if (nparamscheck == LV_MATCHTYPEMASKSTRING) {
		nc->_nparamscheck = nc->_typecheck.size();
	}
	return LV_OK;
}

LVRESULT lv_bindenv(VMHANDLE v, LVInteger idx) {
	LVObjectPtr& o = stack_get(v, idx);
	if (!lv_isnativeclosure(o) && !lv_isclosure(o))
		return lv_throwerror(v, _LC("the target is not a closure"));
	LVObjectPtr& env = stack_get(v, -1);
	if (!lv_istable(env) && !lv_isarray(env) && !lv_isclass(env) && !lv_isinstance(env))
		return lv_throwerror(v, _LC("invalid environment"));
	LVWeakRef *w = _refcounted(env)->GetWeakRef(type(env));
	LVObjectPtr ret;
	if (lv_isclosure(o)) {
		LVClosure *c = _closure(o)->Clone();
		__ObjRelease(c->_env);
		c->_env = w;
		__ObjAddRef(c->_env);
		if (_closure(o)->_base) {
			c->_base = _closure(o)->_base;
			__ObjAddRef(c->_base);
		}
		ret = c;
	} else { //then must be a native closure
		LVNativeClosure *c = _nativeclosure(o)->Clone();
		__ObjRelease(c->_env);
		c->_env = w;
		__ObjAddRef(c->_env);
		ret = c;
	}
	v->Pop();
	v->Push(ret);
	return LV_OK;
}

LVRESULT lv_getclosurename(VMHANDLE v, LVInteger idx) {
	LVObjectPtr& o = stack_get(v, idx);
	if (!lv_isnativeclosure(o) &&
	        !lv_isclosure(o))
		return lv_throwerror(v, _LC("the target is not a closure"));
	if (lv_isnativeclosure(o)) {
		v->Push(_nativeclosure(o)->_name);
	} else { //closure
		v->Push(_closure(o)->_function->_name);
	}
	return LV_OK;
}

LVRESULT lv_setclosureroot(VMHANDLE v, LVInteger idx) {
	LVObjectPtr& c = stack_get(v, idx);
	LVObject o = stack_get(v, -1);
	if (!lv_isclosure(c))
		return lv_throwerror(v, _LC("closure expected"));
	if (lv_istable(o)) {
		_closure(c)->SetRoot(_table(o)->GetWeakRef(OT_TABLE));
		v->Pop();
		return LV_OK;
	}
	return lv_throwerror(v, _LC("ivalid type"));
}

LVRESULT lv_getclosureroot(VMHANDLE v, LVInteger idx) {
	LVObjectPtr& c = stack_get(v, idx);
	if (!lv_isclosure(c))
		return lv_throwerror(v, _LC("closure expected"));
	v->Push(_closure(c)->_root->_obj);
	return LV_OK;
}

LVRESULT lv_clear(VMHANDLE v, LVInteger idx) {
	LVObject& o = stack_get(v, idx);
	switch (type(o)) {
		case OT_TABLE:
			_table(o)->Clear();
			break;
		case OT_ARRAY:
			_array(o)->Resize(0);
			break;
		default:
			return lv_throwerror(v, _LC("clear only works on table and array"));
			break;

	}
	return LV_OK;
}

void lv_pushroottable(VMHANDLE v) {
	v->Push(v->_roottable);
}

void lv_pushregistrytable(VMHANDLE v) {
	v->Push(_ss(v)->_registry);
}

void lv_pushconsttable(VMHANDLE v) {
	v->Push(_ss(v)->_consts);
}

LVRESULT lv_setroottable(VMHANDLE v) {
	LVObject o = stack_get(v, -1);
	if (lv_istable(o) || lv_isnull(o)) {
		v->_roottable = o;
		v->Pop();
		return LV_OK;
	}
	return lv_throwerror(v, _LC("ivalid type"));
}

LVRESULT lv_setconsttable(VMHANDLE v) {
	LVObject o = stack_get(v, -1);
	if (lv_istable(o)) {
		_ss(v)->_consts = o;
		v->Pop();
		return LV_OK;
	}
	return lv_throwerror(v, _LC("ivalid type, expected table"));
}

void lv_setforeignptr(VMHANDLE v, LVUserPointer p) {
	v->_foreignptr = p;
}

LVUserPointer lv_getforeignptr(VMHANDLE v) {
	return v->_foreignptr;
}

void lv_setsharedforeignptr(VMHANDLE v, LVUserPointer p) {
	_ss(v)->_foreignptr = p;
}

LVUserPointer lv_getsharedforeignptr(VMHANDLE v) {
	return _ss(v)->_foreignptr;
}

void lv_setvmreleasehook(VMHANDLE v, LVRELEASEHOOK hook) {
	v->_releasehook = hook;
}

LVRELEASEHOOK lv_getvmreleasehook(VMHANDLE v) {
	return v->_releasehook;
}

void lv_setsharedreleasehook(VMHANDLE v, LVRELEASEHOOK hook) {
	_ss(v)->_releasehook = hook;
}

LVRELEASEHOOK lv_getsharedreleasehook(VMHANDLE v) {
	return _ss(v)->_releasehook;
}

/* Push new item onto the stack */
void lv_push(VMHANDLE v, LVInteger idx) {
	v->Push(stack_get(v, idx));
}

LVObjectType lv_gettype(VMHANDLE v, LVInteger idx) {
	return type(stack_get(v, idx));
}

LVRESULT lv_typeof(VMHANDLE v, LVInteger idx) {
	LVObjectPtr& o = stack_get(v, idx);
	LVObjectPtr res;
	if (!v->TypeOf(o, res)) {
		return LV_ERROR;
	}
	v->Push(res);
	return LV_OK;
}

LVRESULT lv_tostring(VMHANDLE v, LVInteger idx) {
	LVObjectPtr& o = stack_get(v, idx);
	LVObjectPtr res;
	if (!v->ToString(o, res)) {
		return LV_ERROR;
	}
	v->Push(res);
	return LV_OK;
}

void lv_tobool(VMHANDLE v, LVInteger idx, LVBool *b) {
	LVObjectPtr& o = stack_get(v, idx);
	*b = LVVM::IsFalse(o) ? LVFalse : LVTrue;
}

LVRESULT lv_getinteger(VMHANDLE v, LVInteger idx, LVInteger *i) {
	LVObjectPtr& o = stack_get(v, idx);
	if (lv_isnumeric(o)) {
		*i = tointeger(o);
		return LV_OK;
	}
	return LV_ERROR;
}

LVRESULT lv_getfloat(VMHANDLE v, LVInteger idx, LVFloat *f) {
	LVObjectPtr& o = stack_get(v, idx);
	if (lv_isnumeric(o)) {
		*f = tofloat(o);
		return LV_OK;
	}
	return LV_ERROR;
}

LVRESULT lv_getbool(VMHANDLE v, LVInteger idx, LVBool *b) {
	LVObjectPtr& o = stack_get(v, idx);
	if (lv_isbool(o)) {
		*b = _integer(o);
		return LV_OK;
	}
	return LV_ERROR;
}

LVRESULT lv_getstring(VMHANDLE v, LVInteger idx, const LVChar **c) {
	LVObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, OT_STRING, o);
	*c = _stringval(*o);
	return LV_OK;
}

LVRESULT lv_getthread(VMHANDLE v, LVInteger idx, VMHANDLE *thread) {
	LVObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, OT_THREAD, o);
	*thread = _thread(*o);
	return LV_OK;
}

LVRESULT lv_clone(VMHANDLE v, LVInteger idx) {
	LVObjectPtr& o = stack_get(v, idx);
	v->PushNull();
	if (!v->Clone(o, stack_get(v, -1))) {
		v->Pop();
		return LV_ERROR;
	}
	return LV_OK;
}

LVInteger lv_getsize(VMHANDLE v, LVInteger idx) {
	LVObjectPtr& o = stack_get(v, idx);
	LVObjectType type = type(o);
	switch (type) {
		case OT_STRING:
			return _string(o)->_len;
		case OT_TABLE:
			return _table(o)->CountUsed();
		case OT_ARRAY:
			return _array(o)->Size();
		case OT_USERDATA:
			return _userdata(o)->_size;
		case OT_INSTANCE:
			return _instance(o)->_class->_udsize;
		case OT_CLASS:
			return _class(o)->_udsize;
		default:
			return aux_invalidtype(v, type);
	}
}

LVHash lv_gethash(VMHANDLE v, LVInteger idx) {
	LVObjectPtr& o = stack_get(v, idx);
	return HashObj(o);
}

LVRESULT lv_getuserdata(VMHANDLE v, LVInteger idx, LVUserPointer *p, LVUserPointer *typetag) {
	LVObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, OT_USERDATA, o);
	(*p) = _userdataval(*o);
	if (typetag) *typetag = _userdata(*o)->_typetag;
	return LV_OK;
}

LVRESULT lv_settypetag(VMHANDLE v, LVInteger idx, LVUserPointer typetag) {
	LVObjectPtr& o = stack_get(v, idx);
	switch (type(o)) {
		case OT_USERDATA:
			_userdata(o)->_typetag = typetag;
			break;
		case OT_CLASS:
			_class(o)->_typetag = typetag;
			break;
		default:
			return lv_throwerror(v, _LC("invalid object type"));
	}
	return LV_OK;
}

LVRESULT lv_getobjtypetag(const OBJHANDLE *o, LVUserPointer *typetag) {
	switch (type(*o)) {
		case OT_INSTANCE:
			*typetag = _instance(*o)->_class->_typetag;
			break;
		case OT_USERDATA:
			*typetag = _userdata(*o)->_typetag;
			break;
		case OT_CLASS:
			*typetag = _class(*o)->_typetag;
			break;
		default:
			return LV_ERROR;
	}
	return LV_OK;
}

LVRESULT lv_gettypetag(VMHANDLE v, LVInteger idx, LVUserPointer *typetag) {
	LVObjectPtr& o = stack_get(v, idx);
	if (LV_FAILED(lv_getobjtypetag(&o, typetag)))
		return lv_throwerror(v, _LC("invalid object type"));
	return LV_OK;
}

LVRESULT lv_getuserpointer(VMHANDLE v, LVInteger idx, LVUserPointer *p) {
	LVObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, OT_USERPOINTER, o);
	(*p) = _userpointer(*o);
	return LV_OK;
}

/* Return an instance from a user pointer */
LVRESULT lv_setinstanceup(VMHANDLE v, LVInteger idx, LVUserPointer p) {
	LVObjectPtr& o = stack_get(v, idx);
	if (type(o) != OT_INSTANCE)
		return lv_throwerror(v, _LC("the object is not a class instance"));
	_instance(o)->_userpointer = p;
	return LV_OK;
}

LVRESULT lv_setclassudsize(VMHANDLE v, LVInteger idx, LVInteger udsize) {
	LVObjectPtr& o = stack_get(v, idx);
	if (type(o) != OT_CLASS)
		return lv_throwerror(v, _LC("the object is not a class"));
	if (_class(o)->_locked)
		return lv_throwerror(v, _LC("the class is locked"));
	_class(o)->_udsize = udsize;
	return LV_OK;
}

LVRESULT lv_getinstanceup(VMHANDLE v, LVInteger idx, LVUserPointer *p, LVUserPointer typetag) {
	LVObjectPtr& o = stack_get(v, idx);
	if (type(o) != OT_INSTANCE)
		return lv_throwerror(v, _LC("the object is not a class instance"));
	(*p) = _instance(o)->_userpointer;
	if (typetag != 0) {
		LVClass *cl = _instance(o)->_class;
		do {
			if (cl->_typetag == typetag)
				return LV_OK;
			cl = cl->_base;
		} while (cl != NULL);
		return lv_throwerror(v, _LC("invalid type tag"));
	}
	return LV_OK;
}

LVInteger lv_gettop(VMHANDLE v) {
	return (v->_top) - v->_stackbase;
}

void lv_settop(VMHANDLE v, LVInteger newtop) {
	LVInteger top = lv_gettop(v);
	if (top > newtop)
		lv_pop(v, top - newtop);
	else
		while (top++ < newtop)
			lv_pushnull(v);
}

/* Pop items from stack */
void lv_pop(VMHANDLE v, LVInteger nelemstopop) {
	assert(v->_top >= nelemstopop);
	v->Pop(nelemstopop);
}

void lv_poptop(VMHANDLE v) {
	assert(v->_top >= 1);
	v->Pop();
}

void lv_remove(VMHANDLE v, LVInteger idx) {
	v->Remove(idx);
}

LVInteger lv_cmp(VMHANDLE v) {
	LVInteger res;
	v->ObjCmp(stack_get(v, -1), stack_get(v, -2), res);
	return res;
}

LVRESULT lv_newslot(VMHANDLE v, LVInteger idx, LVBool bstatic) {
	aux_paramscheck(v, 3);
	LVObjectPtr& self = stack_get(v, idx);
	if (type(self) == OT_TABLE || type(self) == OT_CLASS) {
		LVObjectPtr& key = v->GetUp(-2);
		if (type(key) == OT_NULL)
			return lv_throwerror(v, _LC("null is not a valid key"));
		v->NewSlot(self, key, v->GetUp(-1), bstatic ? true : false);
		v->Pop(2);
	}
	return LV_OK;
}

LVRESULT lv_deleteslot(VMHANDLE v, LVInteger idx, LVBool pushval) {
	aux_paramscheck(v, 2);
	LVObjectPtr *self;
	_GETSAFE_OBJ(v, idx, OT_TABLE, self);
	LVObjectPtr& key = v->GetUp(-1);
	if (type(key) == OT_NULL)
		return lv_throwerror(v, _LC("null is not a valid key"));
	LVObjectPtr res;
	if (!v->DeleteSlot(*self, key, res)) {
		v->Pop();
		return LV_ERROR;
	}
	if (pushval)
		v->GetUp(-1) = res;
	else
		v->Pop();
	return LV_OK;
}

LVRESULT lv_set(VMHANDLE v, LVInteger idx) {
	LVObjectPtr& self = stack_get(v, idx);
	if (v->Set(self, v->GetUp(-2), v->GetUp(-1), DONT_FALL_BACK)) {
		v->Pop(2);
		return LV_OK;
	}
	return LV_ERROR;
}

LVRESULT lv_rawset(VMHANDLE v, LVInteger idx) {
	LVObjectPtr& self = stack_get(v, idx);
	LVObjectPtr& key = v->GetUp(-2);
	if (type(key) == OT_NULL) {
		v->Pop(2);
		return lv_throwerror(v, _LC("null key"));
	}
	switch (type(self)) {
		case OT_TABLE:
			_table(self)->NewSlot(key, v->GetUp(-1));
			v->Pop(2);
			return LV_OK;
			break;
		case OT_CLASS:
			_class(self)->NewSlot(_ss(v), key, v->GetUp(-1), false);
			v->Pop(2);
			return LV_OK;
			break;
		case OT_INSTANCE:
			if (_instance(self)->Set(key, v->GetUp(-1))) {
				v->Pop(2);
				return LV_OK;
			}
			break;
		case OT_ARRAY:
			if (v->Set(self, key, v->GetUp(-1), false)) {
				v->Pop(2);
				return LV_OK;
			}
			break;
		default:
			v->Pop(2);
			return lv_throwerror(v, _LC("rawset works only on array/table/class and instance"));
	}
	v->Raise_IdxError(v->GetUp(-2));
	return LV_ERROR;
}

LVRESULT lv_newmember(VMHANDLE v, LVInteger idx, LVBool bstatic) {
	LVObjectPtr& self = stack_get(v, idx);
	if (type(self) != OT_CLASS)
		return lv_throwerror(v, _LC("new member only works with classes"));
	LVObjectPtr& key = v->GetUp(-3);
	if (type(key) == OT_NULL)
		return lv_throwerror(v, _LC("null key"));
	if (!v->NewSlotA(self, key, v->GetUp(-2), v->GetUp(-1), bstatic ? true : false, false))
		return LV_ERROR;
	return LV_OK;
}

LVRESULT lv_rawnewmember(VMHANDLE v, LVInteger idx, LVBool bstatic) {
	LVObjectPtr& self = stack_get(v, idx);
	if (type(self) != OT_CLASS)
		return lv_throwerror(v, _LC("new member only works with classes"));
	LVObjectPtr& key = v->GetUp(-3);
	if (type(key) == OT_NULL)
		return lv_throwerror(v, _LC("null key"));
	if (!v->NewSlotA(self, key, v->GetUp(-2), v->GetUp(-1), bstatic ? true : false, true))
		return LV_ERROR;
	return LV_OK;
}

LVRESULT lv_setdelegate(VMHANDLE v, LVInteger idx) {
	LVObjectPtr& self = stack_get(v, idx);
	LVObjectPtr& mt = v->GetUp(-1);
	LVObjectType type = type(self);
	switch (type) {
		case OT_TABLE:
			if (type(mt) == OT_TABLE) {
				if (!_table(self)->SetDelegate(_table(mt))) return lv_throwerror(v, _LC("delagate cycle"));
				v->Pop();
			} else if (type(mt) == OT_NULL) {
				_table(self)->SetDelegate(NULL);
				v->Pop();
			} else return aux_invalidtype(v, type);
			break;
		case OT_USERDATA:
			if (type(mt) == OT_TABLE) {
				_userdata(self)->SetDelegate(_table(mt));
				v->Pop();
			} else if (type(mt) == OT_NULL) {
				_userdata(self)->SetDelegate(NULL);
				v->Pop();
			} else return aux_invalidtype(v, type);
			break;
		default:
			return aux_invalidtype(v, type);
			break;
	}
	return LV_OK;
}

LVRESULT lv_rawdeleteslot(VMHANDLE v, LVInteger idx, LVBool pushval) {
	aux_paramscheck(v, 2);
	LVObjectPtr *self;
	_GETSAFE_OBJ(v, idx, OT_TABLE, self);
	LVObjectPtr& key = v->GetUp(-1);
	LVObjectPtr t;
	if (_table(*self)->Get(key, t)) {
		_table(*self)->Remove(key);
	}
	if (pushval != 0)
		v->GetUp(-1) = t;
	else
		v->Pop();
	return LV_OK;
}

LVRESULT lv_getdelegate(VMHANDLE v, LVInteger idx) {
	LVObjectPtr& self = stack_get(v, idx);
	switch (type(self)) {
		case OT_TABLE:
		case OT_USERDATA:
			if (!_delegable(self)->_delegate) {
				v->PushNull();
				break;
			}
			v->Push(LVObjectPtr(_delegable(self)->_delegate));
			break;
		default:
			return lv_throwerror(v, _LC("wrong type"));
			break;
	}
	return LV_OK;

}

LVRESULT lv_get(VMHANDLE v, LVInteger idx) {
	LVObjectPtr& self = stack_get(v, idx);
	LVObjectPtr& obj = v->GetUp(-1);
	if (v->Get(self, obj, obj, false, DONT_FALL_BACK))
		return LV_OK;
	v->Pop();
	return LV_ERROR;
}

LVRESULT lv_rawget(VMHANDLE v, LVInteger idx) {
	LVObjectPtr& self = stack_get(v, idx);
	LVObjectPtr& obj = v->GetUp(-1);
	switch (type(self)) {
		case OT_TABLE:
			if (_table(self)->Get(obj, obj))
				return LV_OK;
			break;
		case OT_CLASS:
			if (_class(self)->Get(obj, obj))
				return LV_OK;
			break;
		case OT_INSTANCE:
			if (_instance(self)->Get(obj, obj))
				return LV_OK;
			break;
		case OT_ARRAY: {
			if (lv_isnumeric(obj)) {
				if (_array(self)->Get(tointeger(obj), obj)) {
					return LV_OK;
				}
			} else {
				v->Pop();
				return lv_throwerror(v, _LC("invalid index type for an array"));
			}
		}
		break;
		default:
			v->Pop();
			return lv_throwerror(v, _LC("rawget works only on array/table/instance and class"));
	}
	v->Pop();
	return lv_throwerror(v, _LC("the index doesn't exist"));
}

LVRESULT lv_getstackobj(VMHANDLE v, LVInteger idx, OBJHANDLE *po) {
	*po = stack_get(v, idx);
	return LV_OK;
}

const LVChar *lv_getlocal(VMHANDLE v, LVUnsignedInteger level, LVUnsignedInteger idx) {
	LVUnsignedInteger cstksize = v->_callsstacksize;
	LVUnsignedInteger lvl = (cstksize - level) - 1;
	LVInteger stackbase = v->_stackbase;
	if (lvl < cstksize) {
		for (LVUnsignedInteger i = 0; i < level; i++) {
			LVVM::CallInfo& ci = v->_callsstack[(cstksize - i) - 1];
			stackbase -= ci._prevstkbase;
		}
		LVVM::CallInfo& ci = v->_callsstack[lvl];
		if (type(ci._closure) != OT_CLOSURE)
			return NULL;
		LVClosure *c = _closure(ci._closure);
		FunctionPrototype *func = c->_function;
		if (func->_noutervalues > (LVInteger)idx) {
			v->Push(*_outer(c->_outervalues[idx])->_valptr);
			return _stringval(func->_outervalues[idx]._name);
		}
		idx -= func->_noutervalues;
		return func->GetLocal(v, stackbase, idx, (LVInteger)(ci._ip - func->_instructions) - 1);
	}
	return NULL;
}

void lv_pushobject(VMHANDLE v, OBJHANDLE obj) {
	v->Push(LVObjectPtr(obj));
}

void lv_resetobject(OBJHANDLE *po) {
	po->_unVal.pUserPointer = NULL;
	po->_type = OT_NULL;
}

LVRESULT lv_throwerror(VMHANDLE v, const LVChar *err) {
	v->_lasterror = LVString::Create(_ss(v), err);
	return LV_ERROR;
}

LVRESULT lv_throwobject(VMHANDLE v) {
	v->_lasterror = v->GetUp(-1);
	v->Pop();
	return LV_ERROR;
}

void lv_reseterror(VMHANDLE v) {
	v->_lasterror.Null();
}

void lv_getlasterror(VMHANDLE v) {
	v->Push(v->_lasterror);
}

LVRESULT lv_reservestack(VMHANDLE v, LVInteger nsize) {
	if (((LVUnsignedInteger)v->_top + nsize) > v->_stack.size()) {
		if (v->_nmetamethodscall) {
			return lv_throwerror(v, _LC("cannot resize stack while in  a metamethod"));
		}
		v->_stack.resize(v->_stack.size() + ((v->_top + nsize) - v->_stack.size()));
	}
	return LV_OK;
}

LVRESULT lv_resume(VMHANDLE v, LVBool retval, LVBool raiseerror) {
	if (type(v->GetUp(-1)) == OT_GENERATOR) {
		v->PushNull(); //retval
		if (!v->Execute(v->GetUp(-2), 0, v->_top, v->GetUp(-1), raiseerror, LVVM::ET_RESUME_GENERATOR)) {
			v->Raise_Error(v->_lasterror);
			return LV_ERROR;
		}

		if (!retval)
			v->Pop();

		return LV_OK;
	}

	return lv_throwerror(v, _LC("only generators can be resumed"));
}

LVRESULT lv_call(VMHANDLE v, LVInteger params, LVBool retval, LVBool raiseerror) {
	LVObjectPtr res;

	if (v->Call(v->GetUp(-(params + 1)), params, v->_top - params, res, raiseerror ? true : false)) {
		if (!v->_suspended) {
			v->Pop(params); //pop closure and args
		}

		if (retval) {
			v->Push(res);
			return LV_OK;
		}

		return LV_OK;
	} else {
		v->Pop(params);
		return LV_ERROR;
	}

	if (!v->_suspended)
		v->Pop(params);

	return lv_throwerror(v, _LC("call failed"));
}

LVRESULT lv_suspendvm(VMHANDLE v) {
	return v->Suspend();
}

LVRESULT lv_wakeupvm(VMHANDLE v, LVBool wakeupret, LVBool retval, LVBool raiseerror, LVBool throwerror) {
	LVObjectPtr ret;
	if (!v->_suspended)
		return lv_throwerror(v, _LC("cannot resume a vm that is not running any code"));
	LVInteger target = v->_suspended_target;
	if (wakeupret) {
		if (target != -1) {
			v->GetAt(v->_stackbase + v->_suspended_target) = v->GetUp(-1); //retval
		}
		v->Pop();
	} else if (target != -1) {
		v->GetAt(v->_stackbase + v->_suspended_target).Null();
	}
	LVObjectPtr dummy;
	if (!v->Execute(dummy, -1, -1, ret, raiseerror, throwerror ? LVVM::ET_RESUME_THROW_VM : LVVM::ET_RESUME_VM)) {
		return LV_ERROR;
	}
	if (retval)
		v->Push(ret);
	return LV_OK;
}

void lv_setreleasehook(VMHANDLE v, LVInteger idx, LVRELEASEHOOK hook) {
	if (lv_gettop(v) >= 1) {
		LVObjectPtr& ud = stack_get(v, idx);
		switch ( type(ud) ) {
			case OT_USERDATA:
				_userdata(ud)->_hook = hook;
				break;
			case OT_INSTANCE:
				_instance(ud)->_hook = hook;
				break;
			case OT_CLASS:
				_class(ud)->_hook = hook;
				break;
			default:
				break; //shutup compiler
		}
	}
}

LVRELEASEHOOK lv_getreleasehook(VMHANDLE v, LVInteger idx) {
	if (lv_gettop(v) >= 1) {
		LVObjectPtr& ud = stack_get(v, idx);
		switch ( type(ud) ) {
			case OT_USERDATA:
				return _userdata(ud)->_hook;
				break;
			case OT_INSTANCE:
				return _instance(ud)->_hook;
				break;
			case OT_CLASS:
				return _class(ud)->_hook;
				break;
			default:
				break; //shutup compiler
		}
	}
	return NULL;
}

void lv_setcompilererrorhandler(VMHANDLE v, LVCOMPILERERROR f) {
	_ss(v)->_compilererrorhandler = f;
}

LVRESULT lv_writeclosure(VMHANDLE v, LVWRITEFUNC w, LVUserPointer up) {
	LVObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, -1, OT_CLOSURE, o);
	unsigned short tag = BYTECODE_STREAM_TAG;
	if (_closure(*o)->_function->_noutervalues)
		return lv_throwerror(v, _LC("a closure with free valiables bound it cannot be serialized"));
	if (w(up, &tag, 2) != 2)
		return lv_throwerror(v, _LC("io error"));
	if (!_closure(*o)->Save(v, up, w))
		return LV_ERROR;
	return LV_OK;
}

LVRESULT lv_readclosure(VMHANDLE v, LVREADFUNC r, LVUserPointer up) {
	LVObjectPtr closure;

	unsigned short tag;
	if (r(up, &tag, 2) != 2)
		return lv_throwerror(v, _LC("io error"));
	if (tag != BYTECODE_STREAM_TAG)
		return lv_throwerror(v, _LC("invalid stream"));
	if (!LVClosure::Load(v, up, r, closure))
		return LV_ERROR;
	v->Push(closure);
	return LV_OK;
}

LVChar *lv_getscratchpad(VMHANDLE v, LVInteger minsize) {
	return _ss(v)->GetScratchPad(minsize);
}

LVRESULT lv_resurrectunreachable(VMHANDLE v) {
#ifndef NO_GARBAGE_COLLECTOR
	_ss(v)->ResurrectUnreachable(v);
	return LV_OK;
#else
	return lv_throwerror(v, _LC("resurrectunreachable requires a garbage collector build"));
#endif
}

LVInteger lv_collectgarbage(VMHANDLE v) {
#ifndef NO_GARBAGE_COLLECTOR
	return _ss(v)->CollectGarbage(v);
#else
	return -1;
#endif
}

LVRESULT lv_getcallee(VMHANDLE v) {
	if (v->_callsstacksize > 1) {
		v->Push(v->_callsstack[v->_callsstacksize - 2]._closure);
		return LV_OK;
	}
	return lv_throwerror(v, _LC("no closure in the calls stack"));
}

const LVChar *lv_getfreevariable(VMHANDLE v, LVInteger idx, LVUnsignedInteger nval) {
	LVObjectPtr& self = stack_get(v, idx);
	const LVChar *name = NULL;
	switch (type(self)) {
		case OT_CLOSURE: {
			LVClosure *clo = _closure(self);
			FunctionPrototype *fp = clo->_function;
			if (((LVUnsignedInteger)fp->_noutervalues) > nval) {
				v->Push(*(_outer(clo->_outervalues[nval])->_valptr));
				LVOuterVar& ov = fp->_outervalues[nval];
				name = _stringval(ov._name);
			}
		}
		break;
		case OT_NATIVECLOSURE: {
			LVNativeClosure *clo = _nativeclosure(self);
			if (clo->_noutervalues > nval) {
				v->Push(clo->_outervalues[nval]);
				name = _LC("@NATIVE");
			}
		}
		break;
		default:
			break; //shutup compiler
	}
	return name;
}

LVRESULT lv_setfreevariable(VMHANDLE v, LVInteger idx, LVUnsignedInteger nval) {
	LVObjectPtr& self = stack_get(v, idx);
	switch (type(self)) {
		case OT_CLOSURE: {
			FunctionPrototype *fp = _closure(self)->_function;
			if (((LVUnsignedInteger)fp->_noutervalues) > nval) {
				*(_outer(_closure(self)->_outervalues[nval])->_valptr) = stack_get(v, -1);
			} else return lv_throwerror(v, _LC("invalid free var index"));
		}
		break;
		case OT_NATIVECLOSURE:
			if (_nativeclosure(self)->_noutervalues > nval) {
				_nativeclosure(self)->_outervalues[nval] = stack_get(v, -1);
			} else return lv_throwerror(v, _LC("invalid free var index"));
			break;
		default:
			return aux_invalidtype(v, type(self));
	}
	v->Pop();
	return LV_OK;
}

LVRESULT lv_setattributes(VMHANDLE v, LVInteger idx) {
	LVObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, OT_CLASS, o);
	LVObjectPtr& key = stack_get(v, -2);
	LVObjectPtr& val = stack_get(v, -1);
	LVObjectPtr attrs;
	if (type(key) == OT_NULL) {
		attrs = _class(*o)->_attributes;
		_class(*o)->_attributes = val;
		v->Pop(2);
		v->Push(attrs);
		return LV_OK;
	} else if (_class(*o)->GetAttributes(key, attrs)) {
		_class(*o)->SetAttributes(key, val);
		v->Pop(2);
		v->Push(attrs);
		return LV_OK;
	}
	return lv_throwerror(v, _LC("wrong index"));
}

LVRESULT lv_getattributes(VMHANDLE v, LVInteger idx) {
	LVObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, OT_CLASS, o);
	LVObjectPtr& key = stack_get(v, -1);
	LVObjectPtr attrs;
	if (type(key) == OT_NULL) {
		attrs = _class(*o)->_attributes;
		v->Pop();
		v->Push(attrs);
		return LV_OK;
	} else if (_class(*o)->GetAttributes(key, attrs)) {
		v->Pop();
		v->Push(attrs);
		return LV_OK;
	}
	return lv_throwerror(v, _LC("wrong index"));
}

LVRESULT lv_getmemberhandle(VMHANDLE v, LVInteger idx, MEMBERHANDLE *handle) {
	LVObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, OT_CLASS, o);
	LVObjectPtr& key = stack_get(v, -1);
	LVTable *m = _class(*o)->_members;
	LVObjectPtr val;
	if (m->Get(key, val)) {
		handle->_static = _isfield(val) ? LVFalse : LVTrue;
		handle->_index = _member_idx(val);
		v->Pop();
		return LV_OK;
	}
	return lv_throwerror(v, _LC("wrong index"));
}

LVRESULT _getmemberbyhandle(VMHANDLE v, LVObjectPtr& self, const MEMBERHANDLE *handle, LVObjectPtr *&val) {
	switch (type(self)) {
		case OT_INSTANCE: {
			LVInstance *i = _instance(self);
			if (handle->_static) {
				LVClass *c = i->_class;
				val = &c->_methods[handle->_index].val;
			} else {
				val = &i->_values[handle->_index];

			}
		}
		break;
		case OT_CLASS: {
			LVClass *c = _class(self);
			if (handle->_static) {
				val = &c->_methods[handle->_index].val;
			} else {
				val = &c->_defaultvalues[handle->_index].val;
			}
		}
		break;
		default:
			return lv_throwerror(v, _LC("wrong type(expected class or instance)"));
	}
	return LV_OK;
}

LVRESULT lv_getbyhandle(VMHANDLE v, LVInteger idx, const MEMBERHANDLE *handle) {
	LVObjectPtr& self = stack_get(v, idx);
	LVObjectPtr *val = NULL;
	if (LV_FAILED(_getmemberbyhandle(v, self, handle, val))) {
		return LV_ERROR;
	}
	v->Push(_realval(*val));
	return LV_OK;
}

LVRESULT lv_setbyhandle(VMHANDLE v, LVInteger idx, const MEMBERHANDLE *handle) {
	LVObjectPtr& self = stack_get(v, idx);
	LVObjectPtr& newval = stack_get(v, -1);
	LVObjectPtr *val = NULL;
	if (LV_FAILED(_getmemberbyhandle(v, self, handle, val))) {
		return LV_ERROR;
	}
	*val = newval;
	v->Pop();
	return LV_OK;
}

LVRESULT lv_getbase(VMHANDLE v, LVInteger idx) {
	LVObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, OT_CLASS, o);
	if (_class(*o)->_base)
		v->Push(LVObjectPtr(_class(*o)->_base));
	else
		v->PushNull();
	return LV_OK;
}

LVRESULT lv_getclass(VMHANDLE v, LVInteger idx) {
	LVObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, OT_INSTANCE, o);
	v->Push(LVObjectPtr(_instance(*o)->_class));
	return LV_OK;
}

LVRESULT lv_createinstance(VMHANDLE v, LVInteger idx) {
	LVObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, OT_CLASS, o);
	v->Push(_class(*o)->CreateInstance());
	return LV_OK;
}

void lv_weakref(VMHANDLE v, LVInteger idx) {
	LVObject& o = stack_get(v, idx);
	if (ISREFCOUNTED(type(o))) {
		v->Push(_refcounted(o)->GetWeakRef(type(o)));
		return;
	}
	v->Push(o);
}

LVRESULT lv_getweakrefval(VMHANDLE v, LVInteger idx) {
	LVObjectPtr& o = stack_get(v, idx);
	if (type(o) != OT_WEAKREF) {
		return lv_throwerror(v, _LC("the object must be a weakref"));
	}
	v->Push(_weakref(o)->_obj);
	return LV_OK;
}

LVRESULT lv_getdefaultdelegate(VMHANDLE v, LVObjectType t) {
	LVSharedState *ss = _ss(v);
	switch (t) {
		case OT_TABLE:
			v->Push(ss->_table_default_delegate);
			break;
		case OT_ARRAY:
			v->Push(ss->_array_default_delegate);
			break;
		case OT_STRING:
			v->Push(ss->_string_default_delegate);
			break;
		case OT_INTEGER:
		case OT_FLOAT:
			v->Push(ss->_number_default_delegate);
			break;
		case OT_GENERATOR:
			v->Push(ss->_generator_default_delegate);
			break;
		case OT_CLOSURE:
		case OT_NATIVECLOSURE:
			v->Push(ss->_closure_default_delegate);
			break;
		case OT_THREAD:
			v->Push(ss->_thread_default_delegate);
			break;
		case OT_CLASS:
			v->Push(ss->_class_default_delegate);
			break;
		case OT_INSTANCE:
			v->Push(ss->_instance_default_delegate);
			break;
		case OT_WEAKREF:
			v->Push(ss->_weakref_default_delegate);
			break;
		default:
			return lv_throwerror(v, _LC("the type doesn't have a default delegate"));
	}
	return LV_OK;
}

LVRESULT lv_next(VMHANDLE v, LVInteger idx) {
	LVObjectPtr o = stack_get(v, idx), &refpos = stack_get(v, -1), realkey, val;
	if (type(o) == OT_GENERATOR) {
		return lv_throwerror(v, _LC("cannot iterate a generator"));
	}
	int faketojump;
	if (!v->FOREACH_OP(o, realkey, val, refpos, 0, 666, faketojump))
		return LV_ERROR;
	if (faketojump != 666) {
		v->Push(realkey);
		v->Push(val);
		return LV_OK;
	}
	return LV_ERROR;
}

struct BufState {
	const LVChar *buf;
	LVInteger ptr;
	LVInteger size;
};

LVInteger buf_lexfeed(LVUserPointer file) {
	BufState *buf = (BufState *)file;
	if (buf->size < (buf->ptr + 1))
		return 0;
	return buf->buf[buf->ptr++];
}

/* Compile buffer */
LVRESULT lv_compilebuffer(VMHANDLE v, const LVChar *s, LVInteger size, const LVChar *sourcename, LVBool raiseerror) {
	BufState buf;
	buf.buf = s;
	buf.size = size;
	buf.ptr = 0;
	return lv_compile(v, buf_lexfeed, &buf, sourcename, raiseerror);
}

void lv_move(VMHANDLE dest, VMHANDLE src, LVInteger idx) {
	dest->Push(stack_get(src, idx));
}

// #ifdef _DEBUG_DUMP
void lv_stackdump(VMHANDLE v) {
	v->dumpstack(-1, false);
}
// #endif

void lv_setprintfunc(VMHANDLE v, LVPRINTFUNCTION printfunc, LVPRINTFUNCTION errfunc) {
	_ss(v)->_printfunc = printfunc;
	_ss(v)->_errorfunc = errfunc;
}

LVPRINTFUNCTION lv_getprintfunc(VMHANDLE v) {
	return _ss(v)->_printfunc;
}

LVPRINTFUNCTION lv_geterrorfunc(VMHANDLE v) {
	return _ss(v)->_errorfunc;
}

void *lv_malloc(LVUnsignedInteger size) {
	return LV_MALLOC(size);
}

void *lv_realloc(void *p, LVUnsignedInteger oldsize, LVUnsignedInteger newsize) {
	return LV_REALLOC(p, oldsize, newsize);
}

void lv_free(void *p, LVUnsignedInteger size) {
	LV_FREE(p, size);
}
