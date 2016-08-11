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

#ifndef CALLBACK
#define CALLBACK
#endif

#if (defined(_WIN64) || defined(_LP64))
#ifndef _LV64
#define _LV64
#endif
#endif

#define LVTrue  (1)
#define LVFalse (0)

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
#define LVUNICODE
#endif

#include "sqconfig.h"

#define LAVRIL_VERSION    _LC("Lavril 1.1-beta")
#define LAVRIL_COPYRIGHT  _LC("Copyright (C) 2015-2016 Mavicona, Quenza Inc.\nAll Rights Reserved")
#define LAVRIL_AUTHOR     _LC("Quenza Inc.")
#define LAVRIL_VERSION_NUMBER 110

#define LV_VMSTATE_IDLE         0
#define LV_VMSTATE_RUNNING      1
#define LV_VMSTATE_SUSPENDED    2

#define LAVRIL_EOB 0
#define SQ_BYTECODE_STREAM_TAG  0xBEBE

#define OBJECT_REF_COUNTED    0x08000000
#define OBJECT_NUMERIC        0x04000000
#define OBJECT_DELEGABLE      0x02000000
#define OBJECT_CANBEFALSE     0x01000000

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

#define SQ_STREAM_TYPE_TAG 0x80000000

#define init_module(mod,v) mod_init_##mod(v)
#define register_module(mod) LAVRIL_API LVRESULT mod_init_##mod(VMHANDLE v)

/* File operation modes */
#define LV_SEEK_CUR 0
#define LV_SEEK_END 1
#define LV_SEEK_SET 2

typedef enum tagSQObjectType {
	OT_NULL =           (_RT_NULL | OBJECT_CANBEFALSE),
	OT_INTEGER =        (_RT_INTEGER | OBJECT_NUMERIC | OBJECT_CANBEFALSE),
	OT_FLOAT =          (_RT_FLOAT | OBJECT_NUMERIC | OBJECT_CANBEFALSE),
	OT_BOOL =           (_RT_BOOL | OBJECT_CANBEFALSE),
	OT_STRING =         (_RT_STRING | OBJECT_REF_COUNTED),
	OT_TABLE =          (_RT_TABLE | OBJECT_REF_COUNTED | OBJECT_DELEGABLE),
	OT_ARRAY =          (_RT_ARRAY | OBJECT_REF_COUNTED),
	OT_USERDATA =       (_RT_USERDATA | OBJECT_REF_COUNTED | OBJECT_DELEGABLE),
	OT_CLOSURE =        (_RT_CLOSURE | OBJECT_REF_COUNTED),
	OT_NATIVECLOSURE =  (_RT_NATIVECLOSURE | OBJECT_REF_COUNTED),
	OT_GENERATOR =      (_RT_GENERATOR | OBJECT_REF_COUNTED),
	OT_USERPOINTER =    (_RT_USERPOINTER),
	OT_THREAD =         (_RT_THREAD | OBJECT_REF_COUNTED) ,
	OT_FUNCPROTO =      (_RT_FUNCPROTO | OBJECT_REF_COUNTED), /* Internal usage */
	OT_CLASS =          (_RT_CLASS | OBJECT_REF_COUNTED),
	OT_INSTANCE =       (_RT_INSTANCE | OBJECT_REF_COUNTED | OBJECT_DELEGABLE),
	OT_WEAKREF =        (_RT_WEAKREF | OBJECT_REF_COUNTED),
	OT_OUTER =          (_RT_OUTER | OBJECT_REF_COUNTED) /* Internal usage */
} SQObjectType;

#define ISREFCOUNTED(t) (t&OBJECT_REF_COUNTED)

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
	LVUserPointer pUserPointer;
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
	LVBool _static;
	SQInteger _index;
} SQMemberHandle;

typedef struct tagSQStackInfos {
	const SQChar *funcname;
	const SQChar *source;
	SQInteger line;
} SQStackInfos;

typedef struct SQVM *VMHANDLE;
typedef SQObject HSQOBJECT;
typedef SQMemberHandle HSQMEMBERHANDLE;
typedef SQInteger (*SQFUNCTION)(VMHANDLE);
typedef SQInteger (*SQRELEASEHOOK)(LVUserPointer, SQInteger size);
typedef CALLBACK void (*SQCOMPILERERROR)(VMHANDLE, const SQChar * /*desc*/, const SQChar * /*source*/, SQInteger /*line*/, SQInteger /*column*/);
typedef CALLBACK void (*SQPRINTFUNCTION)(VMHANDLE, const SQChar *, ...);
typedef CALLBACK void (*SQDEBUGHOOK)(VMHANDLE /*v*/, SQInteger /*type*/, const SQChar * /*sourcename*/, SQInteger /*line*/, const SQChar * /*funcname*/);
typedef SQInteger (*SQLOADUNIT)(VMHANDLE /*v*/, const SQChar * /*sourcename*/, LVBool /*printerr*/);
typedef SQInteger (*SQWRITEFUNC)(LVUserPointer, LVUserPointer, SQInteger);
typedef SQInteger (*SQREADFUNC)(LVUserPointer, LVUserPointer, SQInteger);
typedef SQInteger (*SQLEXREADFUNC)(LVUserPointer);

typedef struct tagSQRegFunction {
	const SQChar *name;
	SQFUNCTION f;
	SQInteger nparamscheck;
	const SQChar *typemask;
} SQRegFunction;

typedef struct tagSQFunctionInfo {
	LVUserPointer funcid;
	const SQChar *name;
	const SQChar *source;
	SQInteger line;
} SQFunctionInfo;

typedef void *LVFILE;

/* VM */
LAVRIL_API VMHANDLE lv_open(SQInteger initialstacksize);
LAVRIL_API VMHANDLE lv_newthread(VMHANDLE friendvm, SQInteger initialstacksize);
LAVRIL_API void lv_seterrorhandler(VMHANDLE v);
LAVRIL_API void lv_close(VMHANDLE v);
LAVRIL_API void lv_setforeignptr(VMHANDLE v, LVUserPointer p);
LAVRIL_API LVUserPointer lv_getforeignptr(VMHANDLE v);
LAVRIL_API void lv_setsharedforeignptr(VMHANDLE v, LVUserPointer p);
LAVRIL_API LVUserPointer lv_getsharedforeignptr(VMHANDLE v);
LAVRIL_API void lv_setvmreleasehook(VMHANDLE v, SQRELEASEHOOK hook);
LAVRIL_API SQRELEASEHOOK lv_getvmreleasehook(VMHANDLE v);
LAVRIL_API void lv_setsharedreleasehook(VMHANDLE v, SQRELEASEHOOK hook);
LAVRIL_API SQRELEASEHOOK lv_getsharedreleasehook(VMHANDLE v);
LAVRIL_API void lv_setprintfunc(VMHANDLE v, SQPRINTFUNCTION printfunc, SQPRINTFUNCTION errfunc);
LAVRIL_API SQPRINTFUNCTION lv_getprintfunc(VMHANDLE v);
LAVRIL_API SQPRINTFUNCTION lv_geterrorfunc(VMHANDLE v);
LAVRIL_API LVRESULT lv_suspendvm(VMHANDLE v);
LAVRIL_API LVRESULT lv_wakeupvm(VMHANDLE v, LVBool resumedret, LVBool retval, LVBool raiseerror, LVBool throwerror);
LAVRIL_API SQInteger lv_getvmstate(VMHANDLE v);
LAVRIL_API SQInteger lv_getversion();

/* Compiler */
LAVRIL_API LVRESULT lv_compile(VMHANDLE v, SQLEXREADFUNC read, LVUserPointer p, const SQChar *sourcename, LVBool raiseerror);
LAVRIL_API LVRESULT lv_compilebuffer(VMHANDLE v, const SQChar *s, SQInteger size, const SQChar *sourcename, LVBool raiseerror);
LAVRIL_API void lv_enabledebuginfo(VMHANDLE v, LVBool enable);
LAVRIL_API void lv_notifyallexceptions(VMHANDLE v, LVBool enable);
LAVRIL_API void lv_setcompilererrorhandler(VMHANDLE v, SQCOMPILERERROR f);
LAVRIL_API void lv_setunitloader(VMHANDLE v, SQLOADUNIT f);

/* Stack operations */
LAVRIL_API void lv_push(VMHANDLE v, SQInteger idx);
LAVRIL_API void lv_pop(VMHANDLE v, SQInteger nelemstopop);
LAVRIL_API void lv_poptop(VMHANDLE v);
LAVRIL_API void lv_remove(VMHANDLE v, SQInteger idx);
LAVRIL_API SQInteger lv_gettop(VMHANDLE v);
LAVRIL_API void lv_settop(VMHANDLE v, SQInteger newtop);
LAVRIL_API LVRESULT lv_reservestack(VMHANDLE v, SQInteger nsize);
LAVRIL_API SQInteger lv_cmp(VMHANDLE v);
LAVRIL_API void lv_move(VMHANDLE dest, VMHANDLE src, SQInteger idx);
#ifdef _DEBUG
LAVRIL_API void lv_stackdump(VMHANDLE v);
#endif

/* Object creation handling */
LAVRIL_API LVUserPointer lv_newuserdata(VMHANDLE v, SQUnsignedInteger size);
LAVRIL_API void lv_newtable(VMHANDLE v);
LAVRIL_API void lv_newtableex(VMHANDLE v, SQInteger initialcapacity);
LAVRIL_API void lv_newarray(VMHANDLE v, SQInteger size);
LAVRIL_API void lv_newclosure(VMHANDLE v, SQFUNCTION func, SQUnsignedInteger nfreevars);
LAVRIL_API LVRESULT lv_setparamscheck(VMHANDLE v, SQInteger nparamscheck, const SQChar *typemask);
LAVRIL_API LVRESULT lv_bindenv(VMHANDLE v, SQInteger idx);
LAVRIL_API LVRESULT lv_setclosureroot(VMHANDLE v, SQInteger idx);
LAVRIL_API LVRESULT lv_getclosureroot(VMHANDLE v, SQInteger idx);
LAVRIL_API void lv_pushstring(VMHANDLE v, const SQChar *s, SQInteger len);
LAVRIL_API void lv_pushfloat(VMHANDLE v, SQFloat f);
LAVRIL_API void lv_pushinteger(VMHANDLE v, SQInteger n);
LAVRIL_API void lv_pushbool(VMHANDLE v, LVBool b);
LAVRIL_API void lv_pushuserpointer(VMHANDLE v, LVUserPointer p);
LAVRIL_API void lv_pushnull(VMHANDLE v);
LAVRIL_API void lv_pushthread(VMHANDLE v, VMHANDLE thread);
LAVRIL_API SQObjectType lv_gettype(VMHANDLE v, SQInteger idx);
LAVRIL_API LVRESULT lv_typeof(VMHANDLE v, SQInteger idx);
LAVRIL_API SQInteger lv_getsize(VMHANDLE v, SQInteger idx);
LAVRIL_API SQHash lv_gethash(VMHANDLE v, SQInteger idx);
LAVRIL_API LVRESULT lv_getbase(VMHANDLE v, SQInteger idx);
LAVRIL_API LVBool lv_instanceof(VMHANDLE v);
LAVRIL_API LVRESULT lv_tostring(VMHANDLE v, SQInteger idx);
LAVRIL_API void lv_tobool(VMHANDLE v, SQInteger idx, LVBool *b);
LAVRIL_API LVRESULT lv_getstring(VMHANDLE v, SQInteger idx, const SQChar **c);
LAVRIL_API LVRESULT lv_getinteger(VMHANDLE v, SQInteger idx, SQInteger *i);
LAVRIL_API LVRESULT lv_getfloat(VMHANDLE v, SQInteger idx, SQFloat *f);
LAVRIL_API LVRESULT lv_getbool(VMHANDLE v, SQInteger idx, LVBool *b);
LAVRIL_API LVRESULT lv_getthread(VMHANDLE v, SQInteger idx, VMHANDLE *thread);
LAVRIL_API LVRESULT lv_getuserpointer(VMHANDLE v, SQInteger idx, LVUserPointer *p);
LAVRIL_API LVRESULT lv_getuserdata(VMHANDLE v, SQInteger idx, LVUserPointer *p, LVUserPointer *typetag);
LAVRIL_API LVRESULT lv_settypetag(VMHANDLE v, SQInteger idx, LVUserPointer typetag);
LAVRIL_API LVRESULT lv_gettypetag(VMHANDLE v, SQInteger idx, LVUserPointer *typetag);
LAVRIL_API void lv_setreleasehook(VMHANDLE v, SQInteger idx, SQRELEASEHOOK hook);
LAVRIL_API SQRELEASEHOOK lv_getreleasehook(VMHANDLE v, SQInteger idx);
LAVRIL_API SQChar *lv_getscratchpad(VMHANDLE v, SQInteger minsize);
LAVRIL_API LVRESULT lv_getfunctioninfo(VMHANDLE v, SQInteger level, SQFunctionInfo *fi);
LAVRIL_API LVRESULT lv_getclosureinfo(VMHANDLE v, SQInteger idx, SQUnsignedInteger *nparams, SQUnsignedInteger *nfreevars);
LAVRIL_API LVRESULT lv_getclosurename(VMHANDLE v, SQInteger idx);
LAVRIL_API LVRESULT lv_setnativeclosurename(VMHANDLE v, SQInteger idx, const SQChar *name);
LAVRIL_API LVRESULT lv_setinstanceup(VMHANDLE v, SQInteger idx, LVUserPointer p);
LAVRIL_API LVRESULT lv_getinstanceup(VMHANDLE v, SQInteger idx, LVUserPointer *p, LVUserPointer typetag);
LAVRIL_API LVRESULT lv_setclassudsize(VMHANDLE v, SQInteger idx, SQInteger udsize);
LAVRIL_API LVRESULT lv_newclass(VMHANDLE v, LVBool hasbase);
LAVRIL_API LVRESULT lv_createinstance(VMHANDLE v, SQInteger idx);
LAVRIL_API LVRESULT lv_setattributes(VMHANDLE v, SQInteger idx);
LAVRIL_API LVRESULT lv_getattributes(VMHANDLE v, SQInteger idx);
LAVRIL_API LVRESULT lv_getclass(VMHANDLE v, SQInteger idx);
LAVRIL_API void lv_weakref(VMHANDLE v, SQInteger idx);
LAVRIL_API LVRESULT lv_getdefaultdelegate(VMHANDLE v, SQObjectType t);
LAVRIL_API LVRESULT lv_getmemberhandle(VMHANDLE v, SQInteger idx, HSQMEMBERHANDLE *handle);
LAVRIL_API LVRESULT lv_getbyhandle(VMHANDLE v, SQInteger idx, const HSQMEMBERHANDLE *handle);
LAVRIL_API LVRESULT lv_setbyhandle(VMHANDLE v, SQInteger idx, const HSQMEMBERHANDLE *handle);

/* Object manipulation */
LAVRIL_API void lv_pushroottable(VMHANDLE v);
LAVRIL_API void lv_pushregistrytable(VMHANDLE v);
LAVRIL_API void lv_pushconsttable(VMHANDLE v);
LAVRIL_API LVRESULT lv_setroottable(VMHANDLE v);
LAVRIL_API LVRESULT lv_setconsttable(VMHANDLE v);
LAVRIL_API LVRESULT lv_newslot(VMHANDLE v, SQInteger idx, LVBool bstatic);
LAVRIL_API LVRESULT lv_deleteslot(VMHANDLE v, SQInteger idx, LVBool pushval);
LAVRIL_API LVRESULT lv_set(VMHANDLE v, SQInteger idx);
LAVRIL_API LVRESULT lv_get(VMHANDLE v, SQInteger idx);
LAVRIL_API LVRESULT lv_rawget(VMHANDLE v, SQInteger idx);
LAVRIL_API LVRESULT lv_rawset(VMHANDLE v, SQInteger idx);
LAVRIL_API LVRESULT lv_rawdeleteslot(VMHANDLE v, SQInteger idx, LVBool pushval);
LAVRIL_API LVRESULT lv_newmember(VMHANDLE v, SQInteger idx, LVBool bstatic);
LAVRIL_API LVRESULT lv_rawnewmember(VMHANDLE v, SQInteger idx, LVBool bstatic);
LAVRIL_API LVRESULT lv_arrayappend(VMHANDLE v, SQInteger idx);
LAVRIL_API LVRESULT lv_arraypop(VMHANDLE v, SQInteger idx, LVBool pushval);
LAVRIL_API LVRESULT lv_arrayresize(VMHANDLE v, SQInteger idx, SQInteger newsize);
LAVRIL_API LVRESULT lv_arrayreverse(VMHANDLE v, SQInteger idx);
LAVRIL_API LVRESULT lv_arrayremove(VMHANDLE v, SQInteger idx, SQInteger itemidx);
LAVRIL_API LVRESULT lv_arrayinsert(VMHANDLE v, SQInteger idx, SQInteger destpos);
LAVRIL_API LVRESULT lv_setdelegate(VMHANDLE v, SQInteger idx);
LAVRIL_API LVRESULT lv_getdelegate(VMHANDLE v, SQInteger idx);
LAVRIL_API LVRESULT lv_clone(VMHANDLE v, SQInteger idx);
LAVRIL_API LVRESULT lv_setfreevariable(VMHANDLE v, SQInteger idx, SQUnsignedInteger nval);
LAVRIL_API LVRESULT lv_next(VMHANDLE v, SQInteger idx);
LAVRIL_API LVRESULT lv_getweakrefval(VMHANDLE v, SQInteger idx);
LAVRIL_API LVRESULT lv_clear(VMHANDLE v, SQInteger idx);

/* Calls */
LAVRIL_API LVRESULT lv_call(VMHANDLE v, SQInteger params, LVBool retval, LVBool raiseerror);
LAVRIL_API LVRESULT lv_resume(VMHANDLE v, LVBool retval, LVBool raiseerror);
LAVRIL_API const SQChar *lv_getlocal(VMHANDLE v, SQUnsignedInteger level, SQUnsignedInteger idx);
LAVRIL_API LVRESULT lv_getcallee(VMHANDLE v);
LAVRIL_API const SQChar *lv_getfreevariable(VMHANDLE v, SQInteger idx, SQUnsignedInteger nval);
LAVRIL_API LVRESULT lv_throwerror(VMHANDLE v, const SQChar *err);
LAVRIL_API LVRESULT lv_throwobject(VMHANDLE v);
LAVRIL_API void lv_reseterror(VMHANDLE v);
LAVRIL_API void lv_getlasterror(VMHANDLE v);
LAVRIL_API void lv_registererrorhandlers(VMHANDLE v);

/* Raw object handling */
LAVRIL_API LVRESULT lv_getstackobj(VMHANDLE v, SQInteger idx, HSQOBJECT *po);
LAVRIL_API void lv_pushobject(VMHANDLE v, HSQOBJECT obj);
LAVRIL_API void lv_addref(VMHANDLE v, HSQOBJECT *po);
LAVRIL_API LVBool lv_release(VMHANDLE v, HSQOBJECT *po);
LAVRIL_API SQUnsignedInteger lv_getrefcount(VMHANDLE v, HSQOBJECT *po);
LAVRIL_API void lv_resetobject(HSQOBJECT *po);
LAVRIL_API const SQChar *lv_objtostring(const HSQOBJECT *o);
LAVRIL_API LVBool lv_objtobool(const HSQOBJECT *o);
LAVRIL_API SQInteger lv_objtointeger(const HSQOBJECT *o);
LAVRIL_API SQFloat lv_objtofloat(const HSQOBJECT *o);
LAVRIL_API LVUserPointer lv_objtouserpointer(const HSQOBJECT *o);
LAVRIL_API LVRESULT lv_getobjtypetag(const HSQOBJECT *o, LVUserPointer *typetag);
LAVRIL_API SQUnsignedInteger lv_getvmrefcount(VMHANDLE v, const HSQOBJECT *po);

/* GC */
LAVRIL_API SQInteger lv_collectgarbage(VMHANDLE v);
LAVRIL_API LVRESULT lv_resurrectunreachable(VMHANDLE v);

/* Serialization */
LAVRIL_API LVRESULT lv_writeclosure(VMHANDLE vm, SQWRITEFUNC writef, LVUserPointer up);
LAVRIL_API LVRESULT lv_readclosure(VMHANDLE vm, SQREADFUNC readf, LVUserPointer up);

/* Memory */
LAVRIL_API void *lv_malloc(SQUnsignedInteger size);
LAVRIL_API void *lv_realloc(void *p, SQUnsignedInteger oldsize, SQUnsignedInteger newsize);
LAVRIL_API void lv_free(void *p, SQUnsignedInteger size);

/* Debug */
LAVRIL_API LVRESULT lv_stackinfos(VMHANDLE v, SQInteger level, SQStackInfos *si);
LAVRIL_API void lv_setdebughook(VMHANDLE v);
LAVRIL_API void lv_setnativedebughook(VMHANDLE v, SQDEBUGHOOK hook);

/* IO */
LAVRIL_API LVFILE lv_fopen(const SQChar *, const SQChar *);
LAVRIL_API SQInteger lv_fread(LVUserPointer, SQInteger, SQInteger, LVFILE);
LAVRIL_API SQInteger lv_fwrite(const LVUserPointer, SQInteger, SQInteger, LVFILE);
LAVRIL_API SQInteger lv_fseek(LVFILE , SQInteger , SQInteger);
LAVRIL_API SQInteger lv_ftell(LVFILE);
LAVRIL_API SQInteger lv_fflush(LVFILE);
LAVRIL_API SQInteger lv_fclose(LVFILE);
LAVRIL_API SQInteger lv_feof(LVFILE);
LAVRIL_API LVRESULT lv_createfile(VMHANDLE v, LVFILE file, LVBool own);
LAVRIL_API LVRESULT lv_getfile(VMHANDLE v, SQInteger idx, LVFILE *file);

/* IO compiler helpers */
LAVRIL_API LVRESULT lv_loadfile(VMHANDLE v, const SQChar *filename, LVBool printerror);
LAVRIL_API LVRESULT lv_execfile(VMHANDLE v, const SQChar *filename, LVBool retval, LVBool printerror);
LAVRIL_API LVRESULT lv_writeclosuretofile(VMHANDLE v, const SQChar *filename);

/* Blob */
LAVRIL_API LVUserPointer lv_createblob(VMHANDLE v, SQInteger size);
LAVRIL_API LVRESULT lv_getblob(VMHANDLE v, SQInteger idx, LVUserPointer *ptr);
LAVRIL_API SQInteger lv_getblobsize(VMHANDLE v, SQInteger idx);

/* String */
#ifdef REGEX
LAVRIL_API SQRex *sqstd_rex_compile(const SQChar *pattern, const SQChar **error);
LAVRIL_API void sqstd_rex_free(SQRex *exp);
LAVRIL_API LVBool sqstd_rex_match(SQRex *exp, const SQChar *text);
LAVRIL_API LVBool sqstd_rex_search(SQRex *exp, const SQChar *text, const SQChar **out_begin, const SQChar **out_end);
LAVRIL_API LVBool sqstd_rex_searchrange(SQRex *exp, const SQChar *text_begin, const SQChar *text_end, const SQChar **out_begin, const SQChar **out_end);
LAVRIL_API SQInteger sqstd_rex_getsubexpcount(SQRex *exp);
LAVRIL_API LVBool sqstd_rex_getsubexp(SQRex *exp, SQInteger n, SQRexMatch *subexp);
#endif
LAVRIL_API LVRESULT sqstd_format(VMHANDLE v, SQInteger nformatstringidx, SQInteger *outlen, SQChar **output);

/* Modules */
LAVRIL_API LVRESULT mod_init_io(VMHANDLE v);
LAVRIL_API LVRESULT mod_init_blob(VMHANDLE v);
LAVRIL_API LVRESULT mod_init_string(VMHANDLE v);

#include "modules.h"

/* Auxiliary macros */
#define lv_isnumeric(o) ((o)._type & OBJECT_NUMERIC)
#define lv_istable(o) ((o)._type == OT_TABLE)
#define lv_isarray(o) ((o)._type == OT_ARRAY)
#define lv_isfunction(o) ((o)._type == OT_FUNCPROTO)
#define lv_isclosure(o) ((o)._type == OT_CLOSURE)
#define lv_isgenerator(o) ((o)._type == OT_GENERATOR)
#define lv_isnativeclosure(o) ((o)._type == OT_NATIVECLOSURE)
#define lv_isstring(o) ((o)._type == OT_STRING)
#define lv_isinteger(o) ((o)._type == OT_INTEGER)
#define lv_isfloat(o) ((o)._type == OT_FLOAT)
#define lv_isuserpointer(o) ((o)._type == OT_USERPOINTER)
#define lv_isuserdata(o) ((o)._type == OT_USERDATA)
#define lv_isthread(o) ((o)._type == OT_THREAD)
#define lv_isnull(o) ((o)._type == OT_NULL)
#define lv_isclass(o) ((o)._type == OT_CLASS)
#define lv_isinstance(o) ((o)._type == OT_INSTANCE)
#define lv_isbool(o) ((o)._type == OT_BOOL)
#define lv_isweakref(o) ((o)._type == OT_WEAKREF)
#define lv_type(o) ((o)._type)

#define LV_OK (0)
#define LV_ERROR (-1)

#define LV_FAILED(res) (res<0)
#define LV_SUCCEEDED(res) (res>=0)

#ifdef __GNUC__
# define LV_UNUSED_ARG(x) __attribute__((unused)) x
#else
# define LV_UNUSED_ARG(x) ((void)x)
#endif

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*_LAVRIL_H_*/
