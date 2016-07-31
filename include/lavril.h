/*
Copyright (C) 2015-2016 Mavicona, Quenza Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#ifndef _LAVRIL_H_
#define _LAVRIL_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef LAVRIL_API
#define LAVRIL_API extern
#endif

#if (defined(_WIN64) || defined(_LP64))
#ifndef _SQ64
#define _SQ64
#endif
#endif


#define SQTrue  (1)
#define SQFalse (0)

struct SQVM;
struct SQTable;
struct SQArray;
struct SQString;
struct SQClosure;
struct SQGenerator;
struct SQNativeClosure;
struct SQUserData;
struct SQFunctionProto;
struct SQRefCounted;
struct SQClass;
struct SQInstance;
struct SQDelegable;
struct SQOuter;

#ifdef _UNICODE
#define SQUNICODE
#endif

#include "sqconfig.h"

#define SQUIRREL_VERSION    _SC("Lavril 1.0-beta")
#define SQUIRREL_COPYRIGHT  _SC("Copyright (C) 2015-2016 Mavicona, Quenza Inc.\nAll Rights Reserved")
#define SQUIRREL_AUTHOR     _SC("Yorick de Wid")
#define SQUIRREL_VERSION_NUMBER 100

#define SQ_VMSTATE_IDLE         0
#define SQ_VMSTATE_RUNNING      1
#define SQ_VMSTATE_SUSPENDED    2

#define SQUIRREL_EOB 0
#define SQ_BYTECODE_STREAM_TAG  0xBEBE

#define SQOBJECT_REF_COUNTED    0x08000000
#define SQOBJECT_NUMERIC        0x04000000
#define SQOBJECT_DELEGABLE      0x02000000
#define SQOBJECT_CANBEFALSE     0x01000000

#define SQ_MATCHTYPEMASKSTRING (-99999)

#define _RT_MASK 0x00FFFFFF
#define _RAW_TYPE(type) (type&_RT_MASK)

#define _RT_NULL            0x00000001
#define _RT_INTEGER         0x00000002
#define _RT_FLOAT           0x00000004
#define _RT_BOOL            0x00000008
#define _RT_STRING          0x00000010
#define _RT_TABLE           0x00000020
#define _RT_ARRAY           0x00000040
#define _RT_USERDATA        0x00000080
#define _RT_CLOSURE         0x00000100
#define _RT_NATIVECLOSURE   0x00000200
#define _RT_GENERATOR       0x00000400
#define _RT_USERPOINTER     0x00000800
#define _RT_THREAD          0x00001000
#define _RT_FUNCPROTO       0x00002000
#define _RT_CLASS           0x00004000
#define _RT_INSTANCE        0x00008000
#define _RT_WEAKREF         0x00010000
#define _RT_OUTER           0x00020000

typedef enum tagSQObjectType {
	OT_NULL =           (_RT_NULL | SQOBJECT_CANBEFALSE),
	OT_INTEGER =        (_RT_INTEGER | SQOBJECT_NUMERIC | SQOBJECT_CANBEFALSE),
	OT_FLOAT =          (_RT_FLOAT | SQOBJECT_NUMERIC | SQOBJECT_CANBEFALSE),
	OT_BOOL =           (_RT_BOOL | SQOBJECT_CANBEFALSE),
	OT_STRING =         (_RT_STRING | SQOBJECT_REF_COUNTED),
	OT_TABLE =          (_RT_TABLE | SQOBJECT_REF_COUNTED | SQOBJECT_DELEGABLE),
	OT_ARRAY =          (_RT_ARRAY | SQOBJECT_REF_COUNTED),
	OT_USERDATA =       (_RT_USERDATA | SQOBJECT_REF_COUNTED | SQOBJECT_DELEGABLE),
	OT_CLOSURE =        (_RT_CLOSURE | SQOBJECT_REF_COUNTED),
	OT_NATIVECLOSURE =  (_RT_NATIVECLOSURE | SQOBJECT_REF_COUNTED),
	OT_GENERATOR =      (_RT_GENERATOR | SQOBJECT_REF_COUNTED),
	OT_USERPOINTER =    _RT_USERPOINTER,
	OT_THREAD =         (_RT_THREAD | SQOBJECT_REF_COUNTED) ,
	OT_FUNCPROTO =      (_RT_FUNCPROTO | SQOBJECT_REF_COUNTED), //internal usage only
	OT_CLASS =          (_RT_CLASS | SQOBJECT_REF_COUNTED),
	OT_INSTANCE =       (_RT_INSTANCE | SQOBJECT_REF_COUNTED | SQOBJECT_DELEGABLE),
	OT_WEAKREF =        (_RT_WEAKREF | SQOBJECT_REF_COUNTED),
	OT_OUTER =          (_RT_OUTER | SQOBJECT_REF_COUNTED) //internal usage only
} SQObjectType;

#define ISREFCOUNTED(t) (t&SQOBJECT_REF_COUNTED)

typedef union tagSQObjectValue {
	struct SQTable *pTable;
	struct SQArray *pArray;
	struct SQClosure *pClosure;
	struct SQOuter *pOuter;
	struct SQGenerator *pGenerator;
	struct SQNativeClosure *pNativeClosure;
	struct SQString *pString;
	struct SQUserData *pUserData;
	SQInteger nInteger;
	SQFloat fFloat;
	SQUserPointer pUserPointer;
	struct FunctionPrototype *pFunctionProto;
	struct SQRefCounted *pRefCounted;
	struct SQDelegable *pDelegable;
	struct SQVM *pThread;
	struct SQClass *pClass;
	struct SQInstance *pInstance;
	struct SQWeakRef *pWeakRef;
	SQRawObjectVal raw;
} SQObjectValue;

typedef struct tagSQObject {
	SQObjectType _type;
	SQObjectValue _unVal;
} SQObject;

typedef struct  tagSQMemberHandle {
	SQBool _static;
	SQInteger _index;
} SQMemberHandle;

typedef struct tagSQStackInfos {
	const SQChar *funcname;
	const SQChar *source;
	SQInteger line;
} SQStackInfos;

typedef struct SQVM *HSQUIRRELVM;
typedef SQObject HSQOBJECT;
typedef SQMemberHandle HSQMEMBERHANDLE;
typedef SQInteger (*SQFUNCTION)(HSQUIRRELVM);
typedef SQInteger (*SQRELEASEHOOK)(SQUserPointer, SQInteger size);
typedef void (*SQCOMPILERERROR)(HSQUIRRELVM, const SQChar * /*desc*/, const SQChar * /*source*/, SQInteger /*line*/, SQInteger /*column*/);
typedef void (*SQPRINTFUNCTION)(HSQUIRRELVM, const SQChar *, ...);
typedef void (*SQDEBUGHOOK)(HSQUIRRELVM /*v*/, SQInteger /*type*/, const SQChar * /*sourcename*/, SQInteger /*line*/, const SQChar * /*funcname*/);
typedef SQInteger (*SQWRITEFUNC)(SQUserPointer, SQUserPointer, SQInteger);
typedef SQInteger (*SQREADFUNC)(SQUserPointer, SQUserPointer, SQInteger);

typedef SQInteger (*SQLEXREADFUNC)(SQUserPointer);

typedef struct tagSQRegFunction {
	const SQChar *name;
	SQFUNCTION f;
	SQInteger nparamscheck;
	const SQChar *typemask;
} SQRegFunction;

typedef struct tagSQFunctionInfo {
	SQUserPointer funcid;
	const SQChar *name;
	const SQChar *source;
	SQInteger line;
} SQFunctionInfo;

/*vm*/
LAVRIL_API HSQUIRRELVM sq_open(SQInteger initialstacksize);
LAVRIL_API HSQUIRRELVM sq_newthread(HSQUIRRELVM friendvm, SQInteger initialstacksize);
LAVRIL_API void sq_seterrorhandler(HSQUIRRELVM v);
LAVRIL_API void sq_close(HSQUIRRELVM v);
LAVRIL_API void sq_setforeignptr(HSQUIRRELVM v, SQUserPointer p);
LAVRIL_API SQUserPointer sq_getforeignptr(HSQUIRRELVM v);
LAVRIL_API void sq_setsharedforeignptr(HSQUIRRELVM v, SQUserPointer p);
LAVRIL_API SQUserPointer sq_getsharedforeignptr(HSQUIRRELVM v);
LAVRIL_API void sq_setvmreleasehook(HSQUIRRELVM v, SQRELEASEHOOK hook);
LAVRIL_API SQRELEASEHOOK sq_getvmreleasehook(HSQUIRRELVM v);
LAVRIL_API void sq_setsharedreleasehook(HSQUIRRELVM v, SQRELEASEHOOK hook);
LAVRIL_API SQRELEASEHOOK sq_getsharedreleasehook(HSQUIRRELVM v);
LAVRIL_API void sq_setprintfunc(HSQUIRRELVM v, SQPRINTFUNCTION printfunc, SQPRINTFUNCTION errfunc);
LAVRIL_API SQPRINTFUNCTION sq_getprintfunc(HSQUIRRELVM v);
LAVRIL_API SQPRINTFUNCTION sq_geterrorfunc(HSQUIRRELVM v);
LAVRIL_API SQRESULT sq_suspendvm(HSQUIRRELVM v);
LAVRIL_API SQRESULT sq_wakeupvm(HSQUIRRELVM v, SQBool resumedret, SQBool retval, SQBool raiseerror, SQBool throwerror);
LAVRIL_API SQInteger sq_getvmstate(HSQUIRRELVM v);
LAVRIL_API SQInteger sq_getversion();

/*compiler*/
LAVRIL_API SQRESULT sq_compile(HSQUIRRELVM v, SQLEXREADFUNC read, SQUserPointer p, const SQChar *sourcename, SQBool raiseerror);
LAVRIL_API SQRESULT sq_compilebuffer(HSQUIRRELVM v, const SQChar *s, SQInteger size, const SQChar *sourcename, SQBool raiseerror);
LAVRIL_API void sq_enabledebuginfo(HSQUIRRELVM v, SQBool enable);
LAVRIL_API void sq_notifyallexceptions(HSQUIRRELVM v, SQBool enable);
LAVRIL_API void sq_setcompilererrorhandler(HSQUIRRELVM v, SQCOMPILERERROR f);

/*stack operations*/
LAVRIL_API void sq_push(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API void sq_pop(HSQUIRRELVM v, SQInteger nelemstopop);
LAVRIL_API void sq_poptop(HSQUIRRELVM v);
LAVRIL_API void sq_remove(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQInteger sq_gettop(HSQUIRRELVM v);
LAVRIL_API void sq_settop(HSQUIRRELVM v, SQInteger newtop);
LAVRIL_API SQRESULT sq_reservestack(HSQUIRRELVM v, SQInteger nsize);
LAVRIL_API SQInteger sq_cmp(HSQUIRRELVM v);
LAVRIL_API void sq_move(HSQUIRRELVM dest, HSQUIRRELVM src, SQInteger idx);

/*object creation handling*/
LAVRIL_API SQUserPointer sq_newuserdata(HSQUIRRELVM v, SQUnsignedInteger size);
LAVRIL_API void sq_newtable(HSQUIRRELVM v);
LAVRIL_API void sq_newtableex(HSQUIRRELVM v, SQInteger initialcapacity);
LAVRIL_API void sq_newarray(HSQUIRRELVM v, SQInteger size);
LAVRIL_API void sq_newclosure(HSQUIRRELVM v, SQFUNCTION func, SQUnsignedInteger nfreevars);
LAVRIL_API SQRESULT sq_setparamscheck(HSQUIRRELVM v, SQInteger nparamscheck, const SQChar *typemask);
LAVRIL_API SQRESULT sq_bindenv(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQRESULT sq_setclosureroot(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQRESULT sq_getclosureroot(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API void sq_pushstring(HSQUIRRELVM v, const SQChar *s, SQInteger len);
LAVRIL_API void sq_pushfloat(HSQUIRRELVM v, SQFloat f);
LAVRIL_API void sq_pushinteger(HSQUIRRELVM v, SQInteger n);
LAVRIL_API void sq_pushbool(HSQUIRRELVM v, SQBool b);
LAVRIL_API void sq_pushuserpointer(HSQUIRRELVM v, SQUserPointer p);
LAVRIL_API void sq_pushnull(HSQUIRRELVM v);
LAVRIL_API void sq_pushthread(HSQUIRRELVM v, HSQUIRRELVM thread);
LAVRIL_API SQObjectType sq_gettype(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQRESULT sq_typeof(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQInteger sq_getsize(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQHash sq_gethash(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQRESULT sq_getbase(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQBool sq_instanceof(HSQUIRRELVM v);
LAVRIL_API SQRESULT sq_tostring(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API void sq_tobool(HSQUIRRELVM v, SQInteger idx, SQBool *b);
LAVRIL_API SQRESULT sq_getstring(HSQUIRRELVM v, SQInteger idx, const SQChar **c);
LAVRIL_API SQRESULT sq_getinteger(HSQUIRRELVM v, SQInteger idx, SQInteger *i);
LAVRIL_API SQRESULT sq_getfloat(HSQUIRRELVM v, SQInteger idx, SQFloat *f);
LAVRIL_API SQRESULT sq_getbool(HSQUIRRELVM v, SQInteger idx, SQBool *b);
LAVRIL_API SQRESULT sq_getthread(HSQUIRRELVM v, SQInteger idx, HSQUIRRELVM *thread);
LAVRIL_API SQRESULT sq_getuserpointer(HSQUIRRELVM v, SQInteger idx, SQUserPointer *p);
LAVRIL_API SQRESULT sq_getuserdata(HSQUIRRELVM v, SQInteger idx, SQUserPointer *p, SQUserPointer *typetag);
LAVRIL_API SQRESULT sq_settypetag(HSQUIRRELVM v, SQInteger idx, SQUserPointer typetag);
LAVRIL_API SQRESULT sq_gettypetag(HSQUIRRELVM v, SQInteger idx, SQUserPointer *typetag);
LAVRIL_API void sq_setreleasehook(HSQUIRRELVM v, SQInteger idx, SQRELEASEHOOK hook);
LAVRIL_API SQRELEASEHOOK sq_getreleasehook(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQChar *sq_getscratchpad(HSQUIRRELVM v, SQInteger minsize);
LAVRIL_API SQRESULT sq_getfunctioninfo(HSQUIRRELVM v, SQInteger level, SQFunctionInfo *fi);
LAVRIL_API SQRESULT sq_getclosureinfo(HSQUIRRELVM v, SQInteger idx, SQUnsignedInteger *nparams, SQUnsignedInteger *nfreevars);
LAVRIL_API SQRESULT sq_getclosurename(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQRESULT sq_setnativeclosurename(HSQUIRRELVM v, SQInteger idx, const SQChar *name);
LAVRIL_API SQRESULT sq_setinstanceup(HSQUIRRELVM v, SQInteger idx, SQUserPointer p);
LAVRIL_API SQRESULT sq_getinstanceup(HSQUIRRELVM v, SQInteger idx, SQUserPointer *p, SQUserPointer typetag);
LAVRIL_API SQRESULT sq_setclassudsize(HSQUIRRELVM v, SQInteger idx, SQInteger udsize);
LAVRIL_API SQRESULT sq_newclass(HSQUIRRELVM v, SQBool hasbase);
LAVRIL_API SQRESULT sq_createinstance(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQRESULT sq_setattributes(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQRESULT sq_getattributes(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQRESULT sq_getclass(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API void sq_weakref(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQRESULT sq_getdefaultdelegate(HSQUIRRELVM v, SQObjectType t);
LAVRIL_API SQRESULT sq_getmemberhandle(HSQUIRRELVM v, SQInteger idx, HSQMEMBERHANDLE *handle);
LAVRIL_API SQRESULT sq_getbyhandle(HSQUIRRELVM v, SQInteger idx, const HSQMEMBERHANDLE *handle);
LAVRIL_API SQRESULT sq_setbyhandle(HSQUIRRELVM v, SQInteger idx, const HSQMEMBERHANDLE *handle);

/*object manipulation*/
LAVRIL_API void sq_pushroottable(HSQUIRRELVM v);
LAVRIL_API void sq_pushregistrytable(HSQUIRRELVM v);
LAVRIL_API void sq_pushconsttable(HSQUIRRELVM v);
LAVRIL_API SQRESULT sq_setroottable(HSQUIRRELVM v);
LAVRIL_API SQRESULT sq_setconsttable(HSQUIRRELVM v);
LAVRIL_API SQRESULT sq_newslot(HSQUIRRELVM v, SQInteger idx, SQBool bstatic);
LAVRIL_API SQRESULT sq_deleteslot(HSQUIRRELVM v, SQInteger idx, SQBool pushval);
LAVRIL_API SQRESULT sq_set(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQRESULT sq_get(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQRESULT sq_rawget(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQRESULT sq_rawset(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQRESULT sq_rawdeleteslot(HSQUIRRELVM v, SQInteger idx, SQBool pushval);
LAVRIL_API SQRESULT sq_newmember(HSQUIRRELVM v, SQInteger idx, SQBool bstatic);
LAVRIL_API SQRESULT sq_rawnewmember(HSQUIRRELVM v, SQInteger idx, SQBool bstatic);
LAVRIL_API SQRESULT sq_arrayappend(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQRESULT sq_arraypop(HSQUIRRELVM v, SQInteger idx, SQBool pushval);
LAVRIL_API SQRESULT sq_arrayresize(HSQUIRRELVM v, SQInteger idx, SQInteger newsize);
LAVRIL_API SQRESULT sq_arrayreverse(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQRESULT sq_arrayremove(HSQUIRRELVM v, SQInteger idx, SQInteger itemidx);
LAVRIL_API SQRESULT sq_arrayinsert(HSQUIRRELVM v, SQInteger idx, SQInteger destpos);
LAVRIL_API SQRESULT sq_setdelegate(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQRESULT sq_getdelegate(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQRESULT sq_clone(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQRESULT sq_setfreevariable(HSQUIRRELVM v, SQInteger idx, SQUnsignedInteger nval);
LAVRIL_API SQRESULT sq_next(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQRESULT sq_getweakrefval(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQRESULT sq_clear(HSQUIRRELVM v, SQInteger idx);

/*calls*/
LAVRIL_API SQRESULT sq_call(HSQUIRRELVM v, SQInteger params, SQBool retval, SQBool raiseerror);
LAVRIL_API SQRESULT sq_resume(HSQUIRRELVM v, SQBool retval, SQBool raiseerror);
LAVRIL_API const SQChar *sq_getlocal(HSQUIRRELVM v, SQUnsignedInteger level, SQUnsignedInteger idx);
LAVRIL_API SQRESULT sq_getcallee(HSQUIRRELVM v);
LAVRIL_API const SQChar *sq_getfreevariable(HSQUIRRELVM v, SQInteger idx, SQUnsignedInteger nval);
LAVRIL_API SQRESULT sq_throwerror(HSQUIRRELVM v, const SQChar *err);
LAVRIL_API SQRESULT sq_throwobject(HSQUIRRELVM v);
LAVRIL_API void sq_reseterror(HSQUIRRELVM v);
LAVRIL_API void sq_getlasterror(HSQUIRRELVM v);

/*raw object handling*/
LAVRIL_API SQRESULT sq_getstackobj(HSQUIRRELVM v, SQInteger idx, HSQOBJECT *po);
LAVRIL_API void sq_pushobject(HSQUIRRELVM v, HSQOBJECT obj);
LAVRIL_API void sq_addref(HSQUIRRELVM v, HSQOBJECT *po);
LAVRIL_API SQBool sq_release(HSQUIRRELVM v, HSQOBJECT *po);
LAVRIL_API SQUnsignedInteger sq_getrefcount(HSQUIRRELVM v, HSQOBJECT *po);
LAVRIL_API void sq_resetobject(HSQOBJECT *po);
LAVRIL_API const SQChar *sq_objtostring(const HSQOBJECT *o);
LAVRIL_API SQBool sq_objtobool(const HSQOBJECT *o);
LAVRIL_API SQInteger sq_objtointeger(const HSQOBJECT *o);
LAVRIL_API SQFloat sq_objtofloat(const HSQOBJECT *o);
LAVRIL_API SQUserPointer sq_objtouserpointer(const HSQOBJECT *o);
LAVRIL_API SQRESULT sq_getobjtypetag(const HSQOBJECT *o, SQUserPointer *typetag);
LAVRIL_API SQUnsignedInteger sq_getvmrefcount(HSQUIRRELVM v, const HSQOBJECT *po);


/*GC*/
LAVRIL_API SQInteger sq_collectgarbage(HSQUIRRELVM v);
LAVRIL_API SQRESULT sq_resurrectunreachable(HSQUIRRELVM v);

/*serialization*/
LAVRIL_API SQRESULT sq_writeclosure(HSQUIRRELVM vm, SQWRITEFUNC writef, SQUserPointer up);
LAVRIL_API SQRESULT sq_readclosure(HSQUIRRELVM vm, SQREADFUNC readf, SQUserPointer up);

/*mem allocation*/
LAVRIL_API void *sq_malloc(SQUnsignedInteger size);
LAVRIL_API void *sq_realloc(void *p, SQUnsignedInteger oldsize, SQUnsignedInteger newsize);
LAVRIL_API void sq_free(void *p, SQUnsignedInteger size);

/*debug*/
LAVRIL_API SQRESULT sq_stackinfos(HSQUIRRELVM v, SQInteger level, SQStackInfos *si);
LAVRIL_API void sq_setdebughook(HSQUIRRELVM v);
LAVRIL_API void sq_setnativedebughook(HSQUIRRELVM v, SQDEBUGHOOK hook);

/*UTILITY MACRO*/
#define sq_isnumeric(o) ((o)._type&SQOBJECT_NUMERIC)
#define sq_istable(o) ((o)._type==OT_TABLE)
#define sq_isarray(o) ((o)._type==OT_ARRAY)
#define sq_isfunction(o) ((o)._type==OT_FUNCPROTO)
#define sq_isclosure(o) ((o)._type==OT_CLOSURE)
#define sq_isgenerator(o) ((o)._type==OT_GENERATOR)
#define sq_isnativeclosure(o) ((o)._type==OT_NATIVECLOSURE)
#define sq_isstring(o) ((o)._type==OT_STRING)
#define sq_isinteger(o) ((o)._type==OT_INTEGER)
#define sq_isfloat(o) ((o)._type==OT_FLOAT)
#define sq_isuserpointer(o) ((o)._type==OT_USERPOINTER)
#define sq_isuserdata(o) ((o)._type==OT_USERDATA)
#define sq_isthread(o) ((o)._type==OT_THREAD)
#define sq_isnull(o) ((o)._type==OT_NULL)
#define sq_isclass(o) ((o)._type==OT_CLASS)
#define sq_isinstance(o) ((o)._type==OT_INSTANCE)
#define sq_isbool(o) ((o)._type==OT_BOOL)
#define sq_isweakref(o) ((o)._type==OT_WEAKREF)
#define sq_type(o) ((o)._type)

#define LV_OK (0)
#define LV_ERROR (-1)

#define LV_FAILED(res) (res<0)
#define LV_SUCCEEDED(res) (res>=0)

#ifdef __GNUC__
# define SQ_UNUSED_ARG(x) __attribute__((unused)) x
#else
# define SQ_UNUSED_ARG(x) ((void)x)
#endif

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*_LAVRIL_H_*/
