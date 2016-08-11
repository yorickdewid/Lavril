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

static bool sq_aux_gettypedarg(VMHANDLE v, SQInteger idx, SQObjectType type, SQObjectPtr **o) {
	*o = &stack_get(v, idx);
	if (type(**o) != type) {
		SQObjectPtr oval = v->PrintObjVal(**o);
		v->Raise_Error(_LC("wrong argument type, expected '%s' got '%.50s'"), IdType2Name(type), _stringval(oval));
		return false;
	}
	return true;
}

#define _GETSAFE_OBJ(v,idx,type,o) { \
    if(!sq_aux_gettypedarg(v,idx,type,&o)) \
        return LV_ERROR; \
    }

#define lv_aux_paramscheck(v,count) { \
    if(lv_gettop(v) < count) { \
    	v->Raise_Error(_LC("not enough params in the stack")); \
    	return LV_ERROR; \
    }\
}

SQInteger sq_aux_invalidtype(VMHANDLE v, SQObjectType type) {
	SQUnsignedInteger buf_size = 100 * sizeof(SQChar);
	scsprintf(_ss(v)->GetScratchPad(buf_size), buf_size, _LC("unexpected type %s"), IdType2Name(type));
	return lv_throwerror(v, _ss(v)->GetScratchPad(-1));
}

/* Open new VM instance */
VMHANDLE lv_open(SQInteger initialstacksize) {
	SQSharedState *ss = NULL;
	SQVM *v = NULL;
	sq_new(ss, SQSharedState);
	ss->Init();
	v = (SQVM *)SQ_MALLOC(sizeof(SQVM));
	new (v) SQVM(ss);
	ss->_root_vm = v;
	if (v->Init(NULL, initialstacksize)) {
		return v;
	} else {
		sq_delete(v, SQVM);
		return NULL;
	}
	return v;
}

VMHANDLE lv_newthread(VMHANDLE friendvm, SQInteger initialstacksize) {
	SQSharedState *ss = NULL;
	SQVM *v = NULL;
	ss = _ss(friendvm);

	v = (SQVM *)SQ_MALLOC(sizeof(SQVM));
	new (v) SQVM(ss);

	if (v->Init(friendvm, initialstacksize)) {
		friendvm->Push(v);
		return v;
	} else {
		sq_delete(v, SQVM);
		return NULL;
	}
}

SQInteger lv_getvmstate(VMHANDLE v) {
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
	SQObject o = stack_get(v, -1);
	if (lv_isclosure(o) || lv_isnativeclosure(o) || lv_isnull(o)) {
		v->_errorhandler = o;
		v->Pop();
	}
}

void lv_setnativedebughook(VMHANDLE v, SQDEBUGHOOK hook) {
	v->_debughook_native = hook;
	v->_debughook_closure.Null();
	v->_debughook = hook ? true : false;
}

void lv_setdebughook(VMHANDLE v) {
	SQObject o = stack_get(v, -1);
	if (lv_isclosure(o) || lv_isnativeclosure(o) || lv_isnull(o)) {
		v->_debughook_closure = o;
		v->_debughook_native = NULL;
		v->_debughook = !lv_isnull(o);
		v->Pop();
	}
}

/* Release VM instance */
void lv_close(VMHANDLE v) {
	SQSharedState *ss = _ss(v);
	_thread(ss->_root_vm)->Finalize();
	sq_delete(ss, SQSharedState);
}

SQInteger lv_getversion() {
	return LAVRIL_VERSION_NUMBER;
}

/* Compile source */
SQRESULT lv_compile(VMHANDLE v, SQLEXREADFUNC read, SQUserPointer p, const SQChar *sourcename, SQBool raiseerror) {
	SQObjectPtr o;
#ifndef NO_COMPILER
	if (RunCompiler(v, read, p, sourcename, o, raiseerror ? true : false, _ss(v)->_debuginfo)) {
		v->Push(SQClosure::Create(_ss(v), _funcproto(o), _table(v->_roottable)->GetWeakRef(OT_TABLE)));
		return LV_OK;
	}
	return LV_ERROR;
#else
	return lv_throwerror(v, _LC("this is a no compiler build"));
#endif
}

void lv_enabledebuginfo(VMHANDLE v, SQBool enable) {
	_ss(v)->_debuginfo = enable ? true : false;
}

void lv_notifyallexceptions(VMHANDLE v, SQBool enable) {
	_ss(v)->_notifyallexceptions = enable ? true : false;
}

void lv_addref(VMHANDLE v, HSQOBJECT *po) {
	if (!ISREFCOUNTED(type(*po))) return;
#ifdef NO_GARBAGE_COLLECTOR
	__AddRef(po->_type, po->_unVal);
#else
	_ss(v)->_refs_table.AddRef(*po);
#endif
}

SQUnsignedInteger lv_getrefcount(VMHANDLE v, HSQOBJECT *po) {
	if (!ISREFCOUNTED(type(*po))) return 0;
#ifdef NO_GARBAGE_COLLECTOR
	return po->_unVal.pRefCounted->_uiRef;
#else
	return _ss(v)->_refs_table.GetRefCount(*po);
#endif
}

SQBool lv_release(VMHANDLE v, HSQOBJECT *po) {
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

SQUnsignedInteger lv_getvmrefcount(VMHANDLE LV_UNUSED_ARG(v), const HSQOBJECT *po) {
	if (!ISREFCOUNTED(type(*po)))
		return 0;
	return po->_unVal.pRefCounted->_uiRef;
}

const SQChar *lv_objtostring(const HSQOBJECT *o) {
	if (lv_type(*o) == OT_STRING) {
		return _stringval(*o);
	}
	return NULL;
}

SQInteger lv_objtointeger(const HSQOBJECT *o) {
	if (lv_isnumeric(*o)) {
		return tointeger(*o);
	}
	return 0;
}

SQFloat lv_objtofloat(const HSQOBJECT *o) {
	if (lv_isnumeric(*o)) {
		return tofloat(*o);
	}
	return 0;
}

SQBool lv_objtobool(const HSQOBJECT *o) {
	if (lv_isbool(*o)) {
		return _integer(*o);
	}
	return LVFalse;
}

SQUserPointer lv_objtouserpointer(const HSQOBJECT *o) {
	if (lv_isuserpointer(*o)) {
		return _userpointer(*o);
	}
	return 0;
}

void lv_pushnull(VMHANDLE v) {
	v->PushNull();
}

void lv_pushstring(VMHANDLE v, const SQChar *s, SQInteger len) {
	if (s)
		v->Push(SQObjectPtr(SQString::Create(_ss(v), s, len)));
	else
		v->PushNull();
}

void lv_pushinteger(VMHANDLE v, SQInteger n) {
	v->Push(n);
}

void lv_pushbool(VMHANDLE v, SQBool b) {
	v->Push(b ? true : false);
}

void lv_pushfloat(VMHANDLE v, SQFloat n) {
	v->Push(n);
}

void lv_pushuserpointer(VMHANDLE v, SQUserPointer p) {
	v->Push(p);
}

void lv_pushthread(VMHANDLE v, VMHANDLE thread) {
	v->Push(thread);
}

SQUserPointer lv_newuserdata(VMHANDLE v, SQUnsignedInteger size) {
	SQUserData *ud = SQUserData::Create(_ss(v), size + SQ_ALIGNMENT);
	v->Push(ud);
	return (SQUserPointer)sq_aligning(ud + 1);
}

void lv_newtable(VMHANDLE v) {
	v->Push(SQTable::Create(_ss(v), 0));
}

void lv_newtableex(VMHANDLE v, SQInteger initialcapacity) {
	v->Push(SQTable::Create(_ss(v), initialcapacity));
}

void lv_newarray(VMHANDLE v, SQInteger size) {
	v->Push(SQArray::Create(_ss(v), size));
}

SQRESULT lv_newclass(VMHANDLE v, SQBool hasbase) {
	SQClass *baseclass = NULL;
	if (hasbase) {
		SQObjectPtr& base = stack_get(v, -1);
		if (type(base) != OT_CLASS)
			return lv_throwerror(v, _LC("invalid base type"));
		baseclass = _class(base);
	}
	SQClass *newclass = SQClass::Create(_ss(v), baseclass);
	if (baseclass)
		v->Pop();
	v->Push(newclass);
	return LV_OK;
}

SQBool lv_instanceof(VMHANDLE v) {
	SQObjectPtr& inst = stack_get(v, -1);
	SQObjectPtr& cl = stack_get(v, -2);
	if (type(inst) != OT_INSTANCE || type(cl) != OT_CLASS)
		return lv_throwerror(v, _LC("invalid param type"));
	return _instance(inst)->InstanceOf(_class(cl)) ? LVTrue : LVFalse;
}

SQRESULT lv_arrayappend(VMHANDLE v, SQInteger idx) {
	lv_aux_paramscheck(v, 2);
	SQObjectPtr *arr;
	_GETSAFE_OBJ(v, idx, OT_ARRAY, arr);
	_array(*arr)->Append(v->GetUp(-1));
	v->Pop();
	return LV_OK;
}

SQRESULT lv_arraypop(VMHANDLE v, SQInteger idx, SQBool pushval) {
	lv_aux_paramscheck(v, 1);
	SQObjectPtr *arr;
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

SQRESULT lv_arrayresize(VMHANDLE v, SQInteger idx, SQInteger newsize) {
	lv_aux_paramscheck(v, 1);
	SQObjectPtr *arr;
	_GETSAFE_OBJ(v, idx, OT_ARRAY, arr);
	if (newsize >= 0) {
		_array(*arr)->Resize(newsize);
		return LV_OK;
	}
	return lv_throwerror(v, _LC("negative size"));
}

SQRESULT lv_arrayreverse(VMHANDLE v, SQInteger idx) {
	lv_aux_paramscheck(v, 1);
	SQObjectPtr *o;
	_GETSAFE_OBJ(v, idx, OT_ARRAY, o);
	SQArray *arr = _array(*o);
	if (arr->Size() > 0) {
		SQObjectPtr t;
		SQInteger size = arr->Size();
		SQInteger n = size >> 1;
		size -= 1;
		for (SQInteger i = 0; i < n; i++) {
			t = arr->_values[i];
			arr->_values[i] = arr->_values[size - i];
			arr->_values[size - i] = t;
		}
		return LV_OK;
	}
	return LV_OK;
}

SQRESULT lv_arrayremove(VMHANDLE v, SQInteger idx, SQInteger itemidx) {
	lv_aux_paramscheck(v, 1);
	SQObjectPtr *arr;
	_GETSAFE_OBJ(v, idx, OT_ARRAY, arr);
	return _array(*arr)->Remove(itemidx) ? LV_OK : lv_throwerror(v, _LC("index out of range"));
}

SQRESULT lv_arrayinsert(VMHANDLE v, SQInteger idx, SQInteger destpos) {
	lv_aux_paramscheck(v, 1);
	SQObjectPtr *arr;
	_GETSAFE_OBJ(v, idx, OT_ARRAY, arr);
	SQRESULT ret = _array(*arr)->Insert(destpos, v->GetUp(-1)) ? LV_OK : lv_throwerror(v, _LC("index out of range"));
	v->Pop();
	return ret;
}

void lv_newclosure(VMHANDLE v, SQFUNCTION func, SQUnsignedInteger nfreevars) {
	SQNativeClosure *nc = SQNativeClosure::Create(_ss(v), func, nfreevars);
	nc->_nparamscheck = 0;
	for (SQUnsignedInteger i = 0; i < nfreevars; i++) {
		nc->_outervalues[i] = v->Top();
		v->Pop();
	}
	v->Push(SQObjectPtr(nc));
}

SQRESULT lv_getclosureinfo(VMHANDLE v, SQInteger idx, SQUnsignedInteger *nparams, SQUnsignedInteger *nfreevars) {
	SQObject o = stack_get(v, idx);
	if (type(o) == OT_CLOSURE) {
		SQClosure *c = _closure(o);
		FunctionPrototype *proto = c->_function;
		*nparams = (SQUnsignedInteger)proto->_nparameters;
		*nfreevars = (SQUnsignedInteger)proto->_noutervalues;
		return LV_OK;
	} else if (type(o) == OT_NATIVECLOSURE) {
		SQNativeClosure *c = _nativeclosure(o);
		*nparams = (SQUnsignedInteger)c->_nparamscheck;
		*nfreevars = c->_noutervalues;
		return LV_OK;
	}
	return lv_throwerror(v, _LC("the object is not a closure"));
}

SQRESULT lv_setnativeclosurename(VMHANDLE v, SQInteger idx, const SQChar *name) {
	SQObject o = stack_get(v, idx);
	if (lv_isnativeclosure(o)) {
		SQNativeClosure *nc = _nativeclosure(o);
		nc->_name = SQString::Create(_ss(v), name);
		return LV_OK;
	}
	return lv_throwerror(v, _LC("the object is not a nativeclosure"));
}

SQRESULT lv_setparamscheck(VMHANDLE v, SQInteger nparamscheck, const SQChar *typemask) {
	SQObject o = stack_get(v, -1);
	if (!lv_isnativeclosure(o))
		return lv_throwerror(v, _LC("native closure expected"));
	SQNativeClosure *nc = _nativeclosure(o);
	nc->_nparamscheck = nparamscheck;
	if (typemask) {
		SQIntVec res;
		if (!CompileTypemask(res, typemask))
			return lv_throwerror(v, _LC("invalid typemask"));
		nc->_typecheck.copy(res);
	} else {
		nc->_typecheck.resize(0);
	}
	if (nparamscheck == SQ_MATCHTYPEMASKSTRING) {
		nc->_nparamscheck = nc->_typecheck.size();
	}
	return LV_OK;
}

SQRESULT lv_bindenv(VMHANDLE v, SQInteger idx) {
	SQObjectPtr& o = stack_get(v, idx);
	if (!lv_isnativeclosure(o) && !lv_isclosure(o))
		return lv_throwerror(v, _LC("the target is not a closure"));
	SQObjectPtr& env = stack_get(v, -1);
	if (!lv_istable(env) && !lv_isarray(env) && !lv_isclass(env) && !lv_isinstance(env))
		return lv_throwerror(v, _LC("invalid environment"));
	SQWeakRef *w = _refcounted(env)->GetWeakRef(type(env));
	SQObjectPtr ret;
	if (lv_isclosure(o)) {
		SQClosure *c = _closure(o)->Clone();
		__ObjRelease(c->_env);
		c->_env = w;
		__ObjAddRef(c->_env);
		if (_closure(o)->_base) {
			c->_base = _closure(o)->_base;
			__ObjAddRef(c->_base);
		}
		ret = c;
	} else { //then must be a native closure
		SQNativeClosure *c = _nativeclosure(o)->Clone();
		__ObjRelease(c->_env);
		c->_env = w;
		__ObjAddRef(c->_env);
		ret = c;
	}
	v->Pop();
	v->Push(ret);
	return LV_OK;
}

SQRESULT lv_getclosurename(VMHANDLE v, SQInteger idx) {
	SQObjectPtr& o = stack_get(v, idx);
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

SQRESULT lv_setclosureroot(VMHANDLE v, SQInteger idx) {
	SQObjectPtr& c = stack_get(v, idx);
	SQObject o = stack_get(v, -1);
	if (!lv_isclosure(c))
		return lv_throwerror(v, _LC("closure expected"));
	if (lv_istable(o)) {
		_closure(c)->SetRoot(_table(o)->GetWeakRef(OT_TABLE));
		v->Pop();
		return LV_OK;
	}
	return lv_throwerror(v, _LC("ivalid type"));
}

SQRESULT lv_getclosureroot(VMHANDLE v, SQInteger idx) {
	SQObjectPtr& c = stack_get(v, idx);
	if (!lv_isclosure(c))
		return lv_throwerror(v, _LC("closure expected"));
	v->Push(_closure(c)->_root->_obj);
	return LV_OK;
}

SQRESULT lv_clear(VMHANDLE v, SQInteger idx) {
	SQObject& o = stack_get(v, idx);
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

SQRESULT lv_setroottable(VMHANDLE v) {
	SQObject o = stack_get(v, -1);
	if (lv_istable(o) || lv_isnull(o)) {
		v->_roottable = o;
		v->Pop();
		return LV_OK;
	}
	return lv_throwerror(v, _LC("ivalid type"));
}

SQRESULT lv_setconsttable(VMHANDLE v) {
	SQObject o = stack_get(v, -1);
	if (lv_istable(o)) {
		_ss(v)->_consts = o;
		v->Pop();
		return LV_OK;
	}
	return lv_throwerror(v, _LC("ivalid type, expected table"));
}

void lv_setforeignptr(VMHANDLE v, SQUserPointer p) {
	v->_foreignptr = p;
}

SQUserPointer lv_getforeignptr(VMHANDLE v) {
	return v->_foreignptr;
}

void lv_setsharedforeignptr(VMHANDLE v, SQUserPointer p) {
	_ss(v)->_foreignptr = p;
}

SQUserPointer lv_getsharedforeignptr(VMHANDLE v) {
	return _ss(v)->_foreignptr;
}

void lv_setvmreleasehook(VMHANDLE v, SQRELEASEHOOK hook) {
	v->_releasehook = hook;
}

SQRELEASEHOOK lv_getvmreleasehook(VMHANDLE v) {
	return v->_releasehook;
}

void lv_setsharedreleasehook(VMHANDLE v, SQRELEASEHOOK hook) {
	_ss(v)->_releasehook = hook;
}

SQRELEASEHOOK lv_getsharedreleasehook(VMHANDLE v) {
	return _ss(v)->_releasehook;
}

/* Push new item onto the stack */
void lv_push(VMHANDLE v, SQInteger idx) {
	v->Push(stack_get(v, idx));
}

SQObjectType lv_gettype(VMHANDLE v, SQInteger idx) {
	return type(stack_get(v, idx));
}

SQRESULT lv_typeof(VMHANDLE v, SQInteger idx) {
	SQObjectPtr& o = stack_get(v, idx);
	SQObjectPtr res;
	if (!v->TypeOf(o, res)) {
		return LV_ERROR;
	}
	v->Push(res);
	return LV_OK;
}

SQRESULT lv_tostring(VMHANDLE v, SQInteger idx) {
	SQObjectPtr& o = stack_get(v, idx);
	SQObjectPtr res;
	if (!v->ToString(o, res)) {
		return LV_ERROR;
	}
	v->Push(res);
	return LV_OK;
}

void lv_tobool(VMHANDLE v, SQInteger idx, SQBool *b) {
	SQObjectPtr& o = stack_get(v, idx);
	*b = SQVM::IsFalse(o) ? LVFalse : LVTrue;
}

SQRESULT lv_getinteger(VMHANDLE v, SQInteger idx, SQInteger *i) {
	SQObjectPtr& o = stack_get(v, idx);
	if (lv_isnumeric(o)) {
		*i = tointeger(o);
		return LV_OK;
	}
	return LV_ERROR;
}

SQRESULT lv_getfloat(VMHANDLE v, SQInteger idx, SQFloat *f) {
	SQObjectPtr& o = stack_get(v, idx);
	if (lv_isnumeric(o)) {
		*f = tofloat(o);
		return LV_OK;
	}
	return LV_ERROR;
}

SQRESULT lv_getbool(VMHANDLE v, SQInteger idx, SQBool *b) {
	SQObjectPtr& o = stack_get(v, idx);
	if (lv_isbool(o)) {
		*b = _integer(o);
		return LV_OK;
	}
	return LV_ERROR;
}

SQRESULT lv_getstring(VMHANDLE v, SQInteger idx, const SQChar **c) {
	SQObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, OT_STRING, o);
	*c = _stringval(*o);
	return LV_OK;
}

SQRESULT lv_getthread(VMHANDLE v, SQInteger idx, VMHANDLE *thread) {
	SQObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, OT_THREAD, o);
	*thread = _thread(*o);
	return LV_OK;
}

SQRESULT lv_clone(VMHANDLE v, SQInteger idx) {
	SQObjectPtr& o = stack_get(v, idx);
	v->PushNull();
	if (!v->Clone(o, stack_get(v, -1))) {
		v->Pop();
		return LV_ERROR;
	}
	return LV_OK;
}

SQInteger lv_getsize(VMHANDLE v, SQInteger idx) {
	SQObjectPtr& o = stack_get(v, idx);
	SQObjectType type = type(o);
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
			return sq_aux_invalidtype(v, type);
	}
}

SQHash lv_gethash(VMHANDLE v, SQInteger idx) {
	SQObjectPtr& o = stack_get(v, idx);
	return HashObj(o);
}

SQRESULT lv_getuserdata(VMHANDLE v, SQInteger idx, SQUserPointer *p, SQUserPointer *typetag) {
	SQObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, OT_USERDATA, o);
	(*p) = _userdataval(*o);
	if (typetag) *typetag = _userdata(*o)->_typetag;
	return LV_OK;
}

SQRESULT lv_settypetag(VMHANDLE v, SQInteger idx, SQUserPointer typetag) {
	SQObjectPtr& o = stack_get(v, idx);
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

SQRESULT lv_getobjtypetag(const HSQOBJECT *o, SQUserPointer *typetag) {
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

SQRESULT lv_gettypetag(VMHANDLE v, SQInteger idx, SQUserPointer *typetag) {
	SQObjectPtr& o = stack_get(v, idx);
	if (LV_FAILED(lv_getobjtypetag(&o, typetag)))
		return lv_throwerror(v, _LC("invalid object type"));
	return LV_OK;
}

SQRESULT lv_getuserpointer(VMHANDLE v, SQInteger idx, SQUserPointer *p) {
	SQObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, OT_USERPOINTER, o);
	(*p) = _userpointer(*o);
	return LV_OK;
}

SQRESULT lv_setinstanceup(VMHANDLE v, SQInteger idx, SQUserPointer p) {
	SQObjectPtr& o = stack_get(v, idx);
	if (type(o) != OT_INSTANCE)
		return lv_throwerror(v, _LC("the object is not a class instance"));
	_instance(o)->_userpointer = p;
	return LV_OK;
}

SQRESULT lv_setclassudsize(VMHANDLE v, SQInteger idx, SQInteger udsize) {
	SQObjectPtr& o = stack_get(v, idx);
	if (type(o) != OT_CLASS)
		return lv_throwerror(v, _LC("the object is not a class"));
	if (_class(o)->_locked)
		return lv_throwerror(v, _LC("the class is locked"));
	_class(o)->_udsize = udsize;
	return LV_OK;
}

SQRESULT lv_getinstanceup(VMHANDLE v, SQInteger idx, SQUserPointer *p, SQUserPointer typetag) {
	SQObjectPtr& o = stack_get(v, idx);
	if (type(o) != OT_INSTANCE)
		return lv_throwerror(v, _LC("the object is not a class instance"));
	(*p) = _instance(o)->_userpointer;
	if (typetag != 0) {
		SQClass *cl = _instance(o)->_class;
		do {
			if (cl->_typetag == typetag)
				return LV_OK;
			cl = cl->_base;
		} while (cl != NULL);
		return lv_throwerror(v, _LC("invalid type tag"));
	}
	return LV_OK;
}

SQInteger lv_gettop(VMHANDLE v) {
	return (v->_top) - v->_stackbase;
}

void lv_settop(VMHANDLE v, SQInteger newtop) {
	SQInteger top = lv_gettop(v);
	if (top > newtop)
		lv_pop(v, top - newtop);
	else
		while (top++ < newtop)
			lv_pushnull(v);
}

/* Pop items from stack */
void lv_pop(VMHANDLE v, SQInteger nelemstopop) {
	assert(v->_top >= nelemstopop);
	v->Pop(nelemstopop);
}

void lv_poptop(VMHANDLE v) {
	assert(v->_top >= 1);
	v->Pop();
}

void lv_remove(VMHANDLE v, SQInteger idx) {
	v->Remove(idx);
}

SQInteger lv_cmp(VMHANDLE v) {
	SQInteger res;
	v->ObjCmp(stack_get(v, -1), stack_get(v, -2), res);
	return res;
}

SQRESULT lv_newslot(VMHANDLE v, SQInteger idx, SQBool bstatic) {
	lv_aux_paramscheck(v, 3);
	SQObjectPtr& self = stack_get(v, idx);
	if (type(self) == OT_TABLE || type(self) == OT_CLASS) {
		SQObjectPtr& key = v->GetUp(-2);
		if (type(key) == OT_NULL)
			return lv_throwerror(v, _LC("null is not a valid key"));
		v->NewSlot(self, key, v->GetUp(-1), bstatic ? true : false);
		v->Pop(2);
	}
	return LV_OK;
}

SQRESULT lv_deleteslot(VMHANDLE v, SQInteger idx, SQBool pushval) {
	lv_aux_paramscheck(v, 2);
	SQObjectPtr *self;
	_GETSAFE_OBJ(v, idx, OT_TABLE, self);
	SQObjectPtr& key = v->GetUp(-1);
	if (type(key) == OT_NULL)
		return lv_throwerror(v, _LC("null is not a valid key"));
	SQObjectPtr res;
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

SQRESULT lv_set(VMHANDLE v, SQInteger idx) {
	SQObjectPtr& self = stack_get(v, idx);
	if (v->Set(self, v->GetUp(-2), v->GetUp(-1), DONT_FALL_BACK)) {
		v->Pop(2);
		return LV_OK;
	}
	return LV_ERROR;
}

SQRESULT lv_rawset(VMHANDLE v, SQInteger idx) {
	SQObjectPtr& self = stack_get(v, idx);
	SQObjectPtr& key = v->GetUp(-2);
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

SQRESULT lv_newmember(VMHANDLE v, SQInteger idx, SQBool bstatic) {
	SQObjectPtr& self = stack_get(v, idx);
	if (type(self) != OT_CLASS)
		return lv_throwerror(v, _LC("new member only works with classes"));
	SQObjectPtr& key = v->GetUp(-3);
	if (type(key) == OT_NULL)
		return lv_throwerror(v, _LC("null key"));
	if (!v->NewSlotA(self, key, v->GetUp(-2), v->GetUp(-1), bstatic ? true : false, false))
		return LV_ERROR;
	return LV_OK;
}

SQRESULT lv_rawnewmember(VMHANDLE v, SQInteger idx, SQBool bstatic) {
	SQObjectPtr& self = stack_get(v, idx);
	if (type(self) != OT_CLASS)
		return lv_throwerror(v, _LC("new member only works with classes"));
	SQObjectPtr& key = v->GetUp(-3);
	if (type(key) == OT_NULL)
		return lv_throwerror(v, _LC("null key"));
	if (!v->NewSlotA(self, key, v->GetUp(-2), v->GetUp(-1), bstatic ? true : false, true))
		return LV_ERROR;
	return LV_OK;
}

SQRESULT lv_setdelegate(VMHANDLE v, SQInteger idx) {
	SQObjectPtr& self = stack_get(v, idx);
	SQObjectPtr& mt = v->GetUp(-1);
	SQObjectType type = type(self);
	switch (type) {
		case OT_TABLE:
			if (type(mt) == OT_TABLE) {
				if (!_table(self)->SetDelegate(_table(mt))) return lv_throwerror(v, _LC("delagate cycle"));
				v->Pop();
			} else if (type(mt) == OT_NULL) {
				_table(self)->SetDelegate(NULL);
				v->Pop();
			} else return sq_aux_invalidtype(v, type);
			break;
		case OT_USERDATA:
			if (type(mt) == OT_TABLE) {
				_userdata(self)->SetDelegate(_table(mt));
				v->Pop();
			} else if (type(mt) == OT_NULL) {
				_userdata(self)->SetDelegate(NULL);
				v->Pop();
			} else return sq_aux_invalidtype(v, type);
			break;
		default:
			return sq_aux_invalidtype(v, type);
			break;
	}
	return LV_OK;
}

SQRESULT lv_rawdeleteslot(VMHANDLE v, SQInteger idx, SQBool pushval) {
	lv_aux_paramscheck(v, 2);
	SQObjectPtr *self;
	_GETSAFE_OBJ(v, idx, OT_TABLE, self);
	SQObjectPtr& key = v->GetUp(-1);
	SQObjectPtr t;
	if (_table(*self)->Get(key, t)) {
		_table(*self)->Remove(key);
	}
	if (pushval != 0)
		v->GetUp(-1) = t;
	else
		v->Pop();
	return LV_OK;
}

SQRESULT lv_getdelegate(VMHANDLE v, SQInteger idx) {
	SQObjectPtr& self = stack_get(v, idx);
	switch (type(self)) {
		case OT_TABLE:
		case OT_USERDATA:
			if (!_delegable(self)->_delegate) {
				v->PushNull();
				break;
			}
			v->Push(SQObjectPtr(_delegable(self)->_delegate));
			break;
		default:
			return lv_throwerror(v, _LC("wrong type"));
			break;
	}
	return LV_OK;

}

SQRESULT lv_get(VMHANDLE v, SQInteger idx) {
	SQObjectPtr& self = stack_get(v, idx);
	SQObjectPtr& obj = v->GetUp(-1);
	if (v->Get(self, obj, obj, false, DONT_FALL_BACK))
		return LV_OK;
	v->Pop();
	return LV_ERROR;
}

SQRESULT lv_rawget(VMHANDLE v, SQInteger idx) {
	SQObjectPtr& self = stack_get(v, idx);
	SQObjectPtr& obj = v->GetUp(-1);
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

SQRESULT lv_getstackobj(VMHANDLE v, SQInteger idx, HSQOBJECT *po) {
	*po = stack_get(v, idx);
	return LV_OK;
}

const SQChar *lv_getlocal(VMHANDLE v, SQUnsignedInteger level, SQUnsignedInteger idx) {
	SQUnsignedInteger cstksize = v->_callsstacksize;
	SQUnsignedInteger lvl = (cstksize - level) - 1;
	SQInteger stackbase = v->_stackbase;
	if (lvl < cstksize) {
		for (SQUnsignedInteger i = 0; i < level; i++) {
			SQVM::CallInfo& ci = v->_callsstack[(cstksize - i) - 1];
			stackbase -= ci._prevstkbase;
		}
		SQVM::CallInfo& ci = v->_callsstack[lvl];
		if (type(ci._closure) != OT_CLOSURE)
			return NULL;
		SQClosure *c = _closure(ci._closure);
		FunctionPrototype *func = c->_function;
		if (func->_noutervalues > (SQInteger)idx) {
			v->Push(*_outer(c->_outervalues[idx])->_valptr);
			return _stringval(func->_outervalues[idx]._name);
		}
		idx -= func->_noutervalues;
		return func->GetLocal(v, stackbase, idx, (SQInteger)(ci._ip - func->_instructions) - 1);
	}
	return NULL;
}

void lv_pushobject(VMHANDLE v, HSQOBJECT obj) {
	v->Push(SQObjectPtr(obj));
}

void lv_resetobject(HSQOBJECT *po) {
	po->_unVal.pUserPointer = NULL;
	po->_type = OT_NULL;
}

SQRESULT lv_throwerror(VMHANDLE v, const SQChar *err) {
	v->_lasterror = SQString::Create(_ss(v), err);
	return LV_ERROR;
}

SQRESULT lv_throwobject(VMHANDLE v) {
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

SQRESULT lv_reservestack(VMHANDLE v, SQInteger nsize) {
	if (((SQUnsignedInteger)v->_top + nsize) > v->_stack.size()) {
		if (v->_nmetamethodscall) {
			return lv_throwerror(v, _LC("cannot resize stack while in  a metamethod"));
		}
		v->_stack.resize(v->_stack.size() + ((v->_top + nsize) - v->_stack.size()));
	}
	return LV_OK;
}

SQRESULT lv_resume(VMHANDLE v, SQBool retval, SQBool raiseerror) {
	if (type(v->GetUp(-1)) == OT_GENERATOR) {
		v->PushNull(); //retval
		if (!v->Execute(v->GetUp(-2), 0, v->_top, v->GetUp(-1), raiseerror, SQVM::ET_RESUME_GENERATOR)) {
			v->Raise_Error(v->_lasterror);
			return LV_ERROR;
		}

		if (!retval)
			v->Pop();

		return LV_OK;
	}

	return lv_throwerror(v, _LC("only generators can be resumed"));
}

SQRESULT lv_call(VMHANDLE v, SQInteger params, SQBool retval, SQBool raiseerror) {
	SQObjectPtr res;

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

SQRESULT lv_suspendvm(VMHANDLE v) {
	return v->Suspend();
}

SQRESULT lv_wakeupvm(VMHANDLE v, SQBool wakeupret, SQBool retval, SQBool raiseerror, SQBool throwerror) {
	SQObjectPtr ret;
	if (!v->_suspended)
		return lv_throwerror(v, _LC("cannot resume a vm that is not running any code"));
	SQInteger target = v->_suspended_target;
	if (wakeupret) {
		if (target != -1) {
			v->GetAt(v->_stackbase + v->_suspended_target) = v->GetUp(-1); //retval
		}
		v->Pop();
	} else if (target != -1) {
		v->GetAt(v->_stackbase + v->_suspended_target).Null();
	}
	SQObjectPtr dummy;
	if (!v->Execute(dummy, -1, -1, ret, raiseerror, throwerror ? SQVM::ET_RESUME_THROW_VM : SQVM::ET_RESUME_VM)) {
		return LV_ERROR;
	}
	if (retval)
		v->Push(ret);
	return LV_OK;
}

void lv_setreleasehook(VMHANDLE v, SQInteger idx, SQRELEASEHOOK hook) {
	if (lv_gettop(v) >= 1) {
		SQObjectPtr& ud = stack_get(v, idx);
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

SQRELEASEHOOK lv_getreleasehook(VMHANDLE v, SQInteger idx) {
	if (lv_gettop(v) >= 1) {
		SQObjectPtr& ud = stack_get(v, idx);
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

void lv_setcompilererrorhandler(VMHANDLE v, SQCOMPILERERROR f) {
	_ss(v)->_compilererrorhandler = f;
}

void lv_setunitloader(VMHANDLE v, SQLOADUNIT f) {
	_ss(v)->_unitloaderhandler = f;
}

SQRESULT lv_writeclosure(VMHANDLE v, SQWRITEFUNC w, SQUserPointer up) {
	SQObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, -1, OT_CLOSURE, o);
	unsigned short tag = SQ_BYTECODE_STREAM_TAG;
	if (_closure(*o)->_function->_noutervalues)
		return lv_throwerror(v, _LC("a closure with free valiables bound it cannot be serialized"));
	if (w(up, &tag, 2) != 2)
		return lv_throwerror(v, _LC("io error"));
	if (!_closure(*o)->Save(v, up, w))
		return LV_ERROR;
	return LV_OK;
}

SQRESULT lv_readclosure(VMHANDLE v, SQREADFUNC r, SQUserPointer up) {
	SQObjectPtr closure;

	unsigned short tag;
	if (r(up, &tag, 2) != 2)
		return lv_throwerror(v, _LC("io error"));
	if (tag != SQ_BYTECODE_STREAM_TAG)
		return lv_throwerror(v, _LC("invalid stream"));
	if (!SQClosure::Load(v, up, r, closure))
		return LV_ERROR;
	v->Push(closure);
	return LV_OK;
}

SQChar *lv_getscratchpad(VMHANDLE v, SQInteger minsize) {
	return _ss(v)->GetScratchPad(minsize);
}

SQRESULT lv_resurrectunreachable(VMHANDLE v) {
#ifndef NO_GARBAGE_COLLECTOR
	_ss(v)->ResurrectUnreachable(v);
	return LV_OK;
#else
	return lv_throwerror(v, _LC("sq_resurrectunreachable requires a garbage collector build"));
#endif
}

SQInteger lv_collectgarbage(VMHANDLE v) {
#ifndef NO_GARBAGE_COLLECTOR
	return _ss(v)->CollectGarbage(v);
#else
	return -1;
#endif
}

SQRESULT lv_getcallee(VMHANDLE v) {
	if (v->_callsstacksize > 1) {
		v->Push(v->_callsstack[v->_callsstacksize - 2]._closure);
		return LV_OK;
	}
	return lv_throwerror(v, _LC("no closure in the calls stack"));
}

const SQChar *lv_getfreevariable(VMHANDLE v, SQInteger idx, SQUnsignedInteger nval) {
	SQObjectPtr& self = stack_get(v, idx);
	const SQChar *name = NULL;
	switch (type(self)) {
		case OT_CLOSURE: {
			SQClosure *clo = _closure(self);
			FunctionPrototype *fp = clo->_function;
			if (((SQUnsignedInteger)fp->_noutervalues) > nval) {
				v->Push(*(_outer(clo->_outervalues[nval])->_valptr));
				SQOuterVar& ov = fp->_outervalues[nval];
				name = _stringval(ov._name);
			}
		}
		break;
		case OT_NATIVECLOSURE: {
			SQNativeClosure *clo = _nativeclosure(self);
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

SQRESULT lv_setfreevariable(VMHANDLE v, SQInteger idx, SQUnsignedInteger nval) {
	SQObjectPtr& self = stack_get(v, idx);
	switch (type(self)) {
		case OT_CLOSURE: {
			FunctionPrototype *fp = _closure(self)->_function;
			if (((SQUnsignedInteger)fp->_noutervalues) > nval) {
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
			return sq_aux_invalidtype(v, type(self));
	}
	v->Pop();
	return LV_OK;
}

SQRESULT lv_setattributes(VMHANDLE v, SQInteger idx) {
	SQObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, OT_CLASS, o);
	SQObjectPtr& key = stack_get(v, -2);
	SQObjectPtr& val = stack_get(v, -1);
	SQObjectPtr attrs;
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

SQRESULT lv_getattributes(VMHANDLE v, SQInteger idx) {
	SQObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, OT_CLASS, o);
	SQObjectPtr& key = stack_get(v, -1);
	SQObjectPtr attrs;
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

SQRESULT lv_getmemberhandle(VMHANDLE v, SQInteger idx, HSQMEMBERHANDLE *handle) {
	SQObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, OT_CLASS, o);
	SQObjectPtr& key = stack_get(v, -1);
	SQTable *m = _class(*o)->_members;
	SQObjectPtr val;
	if (m->Get(key, val)) {
		handle->_static = _isfield(val) ? LVFalse : LVTrue;
		handle->_index = _member_idx(val);
		v->Pop();
		return LV_OK;
	}
	return lv_throwerror(v, _LC("wrong index"));
}

SQRESULT _getmemberbyhandle(VMHANDLE v, SQObjectPtr& self, const HSQMEMBERHANDLE *handle, SQObjectPtr *&val) {
	switch (type(self)) {
		case OT_INSTANCE: {
			SQInstance *i = _instance(self);
			if (handle->_static) {
				SQClass *c = i->_class;
				val = &c->_methods[handle->_index].val;
			} else {
				val = &i->_values[handle->_index];

			}
		}
		break;
		case OT_CLASS: {
			SQClass *c = _class(self);
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

SQRESULT lv_getbyhandle(VMHANDLE v, SQInteger idx, const HSQMEMBERHANDLE *handle) {
	SQObjectPtr& self = stack_get(v, idx);
	SQObjectPtr *val = NULL;
	if (LV_FAILED(_getmemberbyhandle(v, self, handle, val))) {
		return LV_ERROR;
	}
	v->Push(_realval(*val));
	return LV_OK;
}

SQRESULT lv_setbyhandle(VMHANDLE v, SQInteger idx, const HSQMEMBERHANDLE *handle) {
	SQObjectPtr& self = stack_get(v, idx);
	SQObjectPtr& newval = stack_get(v, -1);
	SQObjectPtr *val = NULL;
	if (LV_FAILED(_getmemberbyhandle(v, self, handle, val))) {
		return LV_ERROR;
	}
	*val = newval;
	v->Pop();
	return LV_OK;
}

SQRESULT lv_getbase(VMHANDLE v, SQInteger idx) {
	SQObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, OT_CLASS, o);
	if (_class(*o)->_base)
		v->Push(SQObjectPtr(_class(*o)->_base));
	else
		v->PushNull();
	return LV_OK;
}

SQRESULT lv_getclass(VMHANDLE v, SQInteger idx) {
	SQObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, OT_INSTANCE, o);
	v->Push(SQObjectPtr(_instance(*o)->_class));
	return LV_OK;
}

SQRESULT lv_createinstance(VMHANDLE v, SQInteger idx) {
	SQObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, OT_CLASS, o);
	v->Push(_class(*o)->CreateInstance());
	return LV_OK;
}

void lv_weakref(VMHANDLE v, SQInteger idx) {
	SQObject& o = stack_get(v, idx);
	if (ISREFCOUNTED(type(o))) {
		v->Push(_refcounted(o)->GetWeakRef(type(o)));
		return;
	}
	v->Push(o);
}

SQRESULT lv_getweakrefval(VMHANDLE v, SQInteger idx) {
	SQObjectPtr& o = stack_get(v, idx);
	if (type(o) != OT_WEAKREF) {
		return lv_throwerror(v, _LC("the object must be a weakref"));
	}
	v->Push(_weakref(o)->_obj);
	return LV_OK;
}

SQRESULT lv_getdefaultdelegate(VMHANDLE v, SQObjectType t) {
	SQSharedState *ss = _ss(v);
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

SQRESULT lv_next(VMHANDLE v, SQInteger idx) {
	SQObjectPtr o = stack_get(v, idx), &refpos = stack_get(v, -1), realkey, val;
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
	const SQChar *buf;
	SQInteger ptr;
	SQInteger size;
};

SQInteger buf_lexfeed(SQUserPointer file) {
	BufState *buf = (BufState *)file;
	if (buf->size < (buf->ptr + 1))
		return 0;
	return buf->buf[buf->ptr++];
}

/* Compile buffer */
SQRESULT lv_compilebuffer(VMHANDLE v, const SQChar *s, SQInteger size, const SQChar *sourcename, SQBool raiseerror) {
	BufState buf;
	buf.buf = s;
	buf.size = size;
	buf.ptr = 0;
	return lv_compile(v, buf_lexfeed, &buf, sourcename, raiseerror);
}

void lv_move(VMHANDLE dest, VMHANDLE src, SQInteger idx) {
	dest->Push(stack_get(src, idx));
}

// #ifdef _DEBUG_DUMP
void lv_stackdump(VMHANDLE v) {
	v->dumpstack(-1, false);
}
// #endif

void lv_setprintfunc(VMHANDLE v, SQPRINTFUNCTION printfunc, SQPRINTFUNCTION errfunc) {
	_ss(v)->_printfunc = printfunc;
	_ss(v)->_errorfunc = errfunc;
}

SQPRINTFUNCTION lv_getprintfunc(VMHANDLE v) {
	return _ss(v)->_printfunc;
}

SQPRINTFUNCTION lv_geterrorfunc(VMHANDLE v) {
	return _ss(v)->_errorfunc;
}

void *lv_malloc(SQUnsignedInteger size) {
	return SQ_MALLOC(size);
}

void *lv_realloc(void *p, SQUnsignedInteger oldsize, SQUnsignedInteger newsize) {
	return SQ_REALLOC(p, oldsize, newsize);
}

void lv_free(void *p, SQUnsignedInteger size) {
	SQ_FREE(p, size);
}
