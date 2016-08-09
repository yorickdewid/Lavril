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

#define LAVRIL_VERSION    _SC("Lavril 1.0-beta")
#define LAVRIL_COPYRIGHT  _SC("Copyright (C) 2015-2016 Mavicona, Quenza Inc.\nAll Rights Reserved")
#define LAVRIL_AUTHOR     _SC("Quenza Inc.")
#define LAVRIL_VERSION_NUMBER 100

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

#define SQ_STREAM_TYPE_TAG 0x80000000

#define init_module(mod,v) mod_init_##mod(v)
#define register_module(mod) LAVRIL_API SQRESULT mod_init_##mod(HSQUIRRELVM v)

struct SQStream {
	virtual SQInteger Read(void *buffer, SQInteger size) = 0;
	virtual SQInteger Write(void *buffer, SQInteger size) = 0;
	virtual SQInteger Flush() = 0;
	virtual SQInteger Tell() = 0;
	virtual SQInteger Len() = 0;
	virtual SQInteger Seek(SQInteger offset, SQInteger origin) = 0;
	virtual bool IsValid() = 0;
	virtual bool EOS() = 0;
};

#define SQ_SEEK_CUR 0
#define SQ_SEEK_END 1
#define SQ_SEEK_SET 2

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
typedef CALLBACK void (*SQCOMPILERERROR)(HSQUIRRELVM, const SQChar * /*desc*/, const SQChar * /*source*/, SQInteger /*line*/, SQInteger /*column*/);
typedef CALLBACK void (*SQPRINTFUNCTION)(HSQUIRRELVM, const SQChar *, ...);
typedef CALLBACK void (*SQDEBUGHOOK)(HSQUIRRELVM /*v*/, SQInteger /*type*/, const SQChar * /*sourcename*/, SQInteger /*line*/, const SQChar * /*funcname*/);
typedef SQInteger (*SQLOADUNIT)(HSQUIRRELVM /*v*/, const SQChar * /*sourcename*/, SQBool /*printerr*/);
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

typedef void *LVFILE;

/* VM */
LAVRIL_API HSQUIRRELVM lv_open(SQInteger initialstacksize);
LAVRIL_API HSQUIRRELVM lv_newthread(HSQUIRRELVM friendvm, SQInteger initialstacksize);
LAVRIL_API void lv_seterrorhandler(HSQUIRRELVM v);
LAVRIL_API void lv_close(HSQUIRRELVM v);
LAVRIL_API void lv_setforeignptr(HSQUIRRELVM v, SQUserPointer p);
LAVRIL_API SQUserPointer lv_getforeignptr(HSQUIRRELVM v);
LAVRIL_API void lv_setsharedforeignptr(HSQUIRRELVM v, SQUserPointer p);
LAVRIL_API SQUserPointer lv_getsharedforeignptr(HSQUIRRELVM v);
LAVRIL_API void lv_setvmreleasehook(HSQUIRRELVM v, SQRELEASEHOOK hook);
LAVRIL_API SQRELEASEHOOK lv_getvmreleasehook(HSQUIRRELVM v);
LAVRIL_API void lv_setsharedreleasehook(HSQUIRRELVM v, SQRELEASEHOOK hook);
LAVRIL_API SQRELEASEHOOK lv_getsharedreleasehook(HSQUIRRELVM v);
LAVRIL_API void lv_setprintfunc(HSQUIRRELVM v, SQPRINTFUNCTION printfunc, SQPRINTFUNCTION errfunc);
LAVRIL_API SQPRINTFUNCTION lv_getprintfunc(HSQUIRRELVM v);
LAVRIL_API SQPRINTFUNCTION lv_geterrorfunc(HSQUIRRELVM v);
LAVRIL_API SQRESULT lv_suspendvm(HSQUIRRELVM v);
LAVRIL_API SQRESULT lv_wakeupvm(HSQUIRRELVM v, SQBool resumedret, SQBool retval, SQBool raiseerror, SQBool throwerror);
LAVRIL_API SQInteger lv_getvmstate(HSQUIRRELVM v);
LAVRIL_API SQInteger lv_getversion();

/* Compiler */
LAVRIL_API SQRESULT lv_compile(HSQUIRRELVM v, SQLEXREADFUNC read, SQUserPointer p, const SQChar *sourcename, SQBool raiseerror);
LAVRIL_API SQRESULT lv_compilebuffer(HSQUIRRELVM v, const SQChar *s, SQInteger size, const SQChar *sourcename, SQBool raiseerror);
LAVRIL_API void lv_enabledebuginfo(HSQUIRRELVM v, SQBool enable);
LAVRIL_API void lv_notifyallexceptions(HSQUIRRELVM v, SQBool enable);
LAVRIL_API void lv_setcompilererrorhandler(HSQUIRRELVM v, SQCOMPILERERROR f);
LAVRIL_API void lv_setunitloader(HSQUIRRELVM v, SQLOADUNIT f);

/* Stack operations */
LAVRIL_API void lv_push(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API void lv_pop(HSQUIRRELVM v, SQInteger nelemstopop);
LAVRIL_API void lv_poptop(HSQUIRRELVM v);
LAVRIL_API void lv_remove(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQInteger lv_gettop(HSQUIRRELVM v);
LAVRIL_API void lv_settop(HSQUIRRELVM v, SQInteger newtop);
LAVRIL_API SQRESULT lv_reservestack(HSQUIRRELVM v, SQInteger nsize);
LAVRIL_API SQInteger lv_cmp(HSQUIRRELVM v);
LAVRIL_API void lv_move(HSQUIRRELVM dest, HSQUIRRELVM src, SQInteger idx);
#ifdef _DEBUG
LAVRIL_API void lv_stackdump(HSQUIRRELVM v);
#endif

/* Object creation handling */
LAVRIL_API SQUserPointer lv_newuserdata(HSQUIRRELVM v, SQUnsignedInteger size);
LAVRIL_API void lv_newtable(HSQUIRRELVM v);
LAVRIL_API void lv_newtableex(HSQUIRRELVM v, SQInteger initialcapacity);
LAVRIL_API void lv_newarray(HSQUIRRELVM v, SQInteger size);
LAVRIL_API void lv_newclosure(HSQUIRRELVM v, SQFUNCTION func, SQUnsignedInteger nfreevars);
LAVRIL_API SQRESULT lv_setparamscheck(HSQUIRRELVM v, SQInteger nparamscheck, const SQChar *typemask);
LAVRIL_API SQRESULT lv_bindenv(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQRESULT lv_setclosureroot(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQRESULT lv_getclosureroot(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API void lv_pushstring(HSQUIRRELVM v, const SQChar *s, SQInteger len);
LAVRIL_API void lv_pushfloat(HSQUIRRELVM v, SQFloat f);
LAVRIL_API void lv_pushinteger(HSQUIRRELVM v, SQInteger n);
LAVRIL_API void lv_pushbool(HSQUIRRELVM v, SQBool b);
LAVRIL_API void lv_pushuserpointer(HSQUIRRELVM v, SQUserPointer p);
LAVRIL_API void lv_pushnull(HSQUIRRELVM v);
LAVRIL_API void lv_pushthread(HSQUIRRELVM v, HSQUIRRELVM thread);
LAVRIL_API SQObjectType lv_gettype(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQRESULT lv_typeof(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQInteger lv_getsize(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQHash lv_gethash(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQRESULT lv_getbase(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQBool lv_instanceof(HSQUIRRELVM v);
LAVRIL_API SQRESULT lv_tostring(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API void lv_tobool(HSQUIRRELVM v, SQInteger idx, SQBool *b);
LAVRIL_API SQRESULT lv_getstring(HSQUIRRELVM v, SQInteger idx, const SQChar **c);
LAVRIL_API SQRESULT lv_getinteger(HSQUIRRELVM v, SQInteger idx, SQInteger *i);
LAVRIL_API SQRESULT lv_getfloat(HSQUIRRELVM v, SQInteger idx, SQFloat *f);
LAVRIL_API SQRESULT lv_getbool(HSQUIRRELVM v, SQInteger idx, SQBool *b);
LAVRIL_API SQRESULT lv_getthread(HSQUIRRELVM v, SQInteger idx, HSQUIRRELVM *thread);
LAVRIL_API SQRESULT lv_getuserpointer(HSQUIRRELVM v, SQInteger idx, SQUserPointer *p);
LAVRIL_API SQRESULT lv_getuserdata(HSQUIRRELVM v, SQInteger idx, SQUserPointer *p, SQUserPointer *typetag);
LAVRIL_API SQRESULT lv_settypetag(HSQUIRRELVM v, SQInteger idx, SQUserPointer typetag);
LAVRIL_API SQRESULT lv_gettypetag(HSQUIRRELVM v, SQInteger idx, SQUserPointer *typetag);
LAVRIL_API void lv_setreleasehook(HSQUIRRELVM v, SQInteger idx, SQRELEASEHOOK hook);
LAVRIL_API SQRELEASEHOOK lv_getreleasehook(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQChar *lv_getscratchpad(HSQUIRRELVM v, SQInteger minsize);
LAVRIL_API SQRESULT lv_getfunctioninfo(HSQUIRRELVM v, SQInteger level, SQFunctionInfo *fi);
LAVRIL_API SQRESULT lv_getclosureinfo(HSQUIRRELVM v, SQInteger idx, SQUnsignedInteger *nparams, SQUnsignedInteger *nfreevars);
LAVRIL_API SQRESULT lv_getclosurename(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQRESULT lv_setnativeclosurename(HSQUIRRELVM v, SQInteger idx, const SQChar *name);
LAVRIL_API SQRESULT lv_setinstanceup(HSQUIRRELVM v, SQInteger idx, SQUserPointer p);
LAVRIL_API SQRESULT lv_getinstanceup(HSQUIRRELVM v, SQInteger idx, SQUserPointer *p, SQUserPointer typetag);
LAVRIL_API SQRESULT lv_setclassudsize(HSQUIRRELVM v, SQInteger idx, SQInteger udsize);
LAVRIL_API SQRESULT lv_newclass(HSQUIRRELVM v, SQBool hasbase);
LAVRIL_API SQRESULT lv_createinstance(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQRESULT lv_setattributes(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQRESULT lv_getattributes(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQRESULT lv_getclass(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API void lv_weakref(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQRESULT lv_getdefaultdelegate(HSQUIRRELVM v, SQObjectType t);
LAVRIL_API SQRESULT lv_getmemberhandle(HSQUIRRELVM v, SQInteger idx, HSQMEMBERHANDLE *handle);
LAVRIL_API SQRESULT lv_getbyhandle(HSQUIRRELVM v, SQInteger idx, const HSQMEMBERHANDLE *handle);
LAVRIL_API SQRESULT lv_setbyhandle(HSQUIRRELVM v, SQInteger idx, const HSQMEMBERHANDLE *handle);

/* Object manipulation */
LAVRIL_API void lv_pushroottable(HSQUIRRELVM v);
LAVRIL_API void lv_pushregistrytable(HSQUIRRELVM v);
LAVRIL_API void lv_pushconsttable(HSQUIRRELVM v);
LAVRIL_API SQRESULT lv_setroottable(HSQUIRRELVM v);
LAVRIL_API SQRESULT lv_setconsttable(HSQUIRRELVM v);
LAVRIL_API SQRESULT lv_newslot(HSQUIRRELVM v, SQInteger idx, SQBool bstatic);
LAVRIL_API SQRESULT lv_deleteslot(HSQUIRRELVM v, SQInteger idx, SQBool pushval);
LAVRIL_API SQRESULT lv_set(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQRESULT lv_get(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQRESULT lv_rawget(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQRESULT lv_rawset(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQRESULT lv_rawdeleteslot(HSQUIRRELVM v, SQInteger idx, SQBool pushval);
LAVRIL_API SQRESULT lv_newmember(HSQUIRRELVM v, SQInteger idx, SQBool bstatic);
LAVRIL_API SQRESULT lv_rawnewmember(HSQUIRRELVM v, SQInteger idx, SQBool bstatic);
LAVRIL_API SQRESULT lv_arrayappend(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQRESULT lv_arraypop(HSQUIRRELVM v, SQInteger idx, SQBool pushval);
LAVRIL_API SQRESULT lv_arrayresize(HSQUIRRELVM v, SQInteger idx, SQInteger newsize);
LAVRIL_API SQRESULT lv_arrayreverse(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQRESULT lv_arrayremove(HSQUIRRELVM v, SQInteger idx, SQInteger itemidx);
LAVRIL_API SQRESULT lv_arrayinsert(HSQUIRRELVM v, SQInteger idx, SQInteger destpos);
LAVRIL_API SQRESULT lv_setdelegate(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQRESULT lv_getdelegate(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQRESULT lv_clone(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQRESULT lv_setfreevariable(HSQUIRRELVM v, SQInteger idx, SQUnsignedInteger nval);
LAVRIL_API SQRESULT lv_next(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQRESULT lv_getweakrefval(HSQUIRRELVM v, SQInteger idx);
LAVRIL_API SQRESULT lv_clear(HSQUIRRELVM v, SQInteger idx);

/* Calls */
LAVRIL_API SQRESULT lv_call(HSQUIRRELVM v, SQInteger params, SQBool retval, SQBool raiseerror);
LAVRIL_API SQRESULT lv_resume(HSQUIRRELVM v, SQBool retval, SQBool raiseerror);
LAVRIL_API const SQChar *lv_getlocal(HSQUIRRELVM v, SQUnsignedInteger level, SQUnsignedInteger idx);
LAVRIL_API SQRESULT lv_getcallee(HSQUIRRELVM v);
LAVRIL_API const SQChar *lv_getfreevariable(HSQUIRRELVM v, SQInteger idx, SQUnsignedInteger nval);
LAVRIL_API SQRESULT lv_throwerror(HSQUIRRELVM v, const SQChar *err);
LAVRIL_API SQRESULT lv_throwobject(HSQUIRRELVM v);
LAVRIL_API void lv_reseterror(HSQUIRRELVM v);
LAVRIL_API void lv_getlasterror(HSQUIRRELVM v);
LAVRIL_API void lv_registererrorhandlers(HSQUIRRELVM v);

/* Raw object handling */
LAVRIL_API SQRESULT lv_getstackobj(HSQUIRRELVM v, SQInteger idx, HSQOBJECT *po);
LAVRIL_API void lv_pushobject(HSQUIRRELVM v, HSQOBJECT obj);
LAVRIL_API void lv_addref(HSQUIRRELVM v, HSQOBJECT *po);
LAVRIL_API SQBool lv_release(HSQUIRRELVM v, HSQOBJECT *po);
LAVRIL_API SQUnsignedInteger lv_getrefcount(HSQUIRRELVM v, HSQOBJECT *po);
LAVRIL_API void lv_resetobject(HSQOBJECT *po);
LAVRIL_API const SQChar *lv_objtostring(const HSQOBJECT *o);
LAVRIL_API SQBool lv_objtobool(const HSQOBJECT *o);
LAVRIL_API SQInteger lv_objtointeger(const HSQOBJECT *o);
LAVRIL_API SQFloat lv_objtofloat(const HSQOBJECT *o);
LAVRIL_API SQUserPointer lv_objtouserpointer(const HSQOBJECT *o);
LAVRIL_API SQRESULT lv_getobjtypetag(const HSQOBJECT *o, SQUserPointer *typetag);
LAVRIL_API SQUnsignedInteger lv_getvmrefcount(HSQUIRRELVM v, const HSQOBJECT *po);

/* GC */
LAVRIL_API SQInteger lv_collectgarbage(HSQUIRRELVM v);
LAVRIL_API SQRESULT lv_resurrectunreachable(HSQUIRRELVM v);

/* Serialization */
LAVRIL_API SQRESULT lv_writeclosure(HSQUIRRELVM vm, SQWRITEFUNC writef, SQUserPointer up);
LAVRIL_API SQRESULT lv_readclosure(HSQUIRRELVM vm, SQREADFUNC readf, SQUserPointer up);

/* Memory */
LAVRIL_API void *lv_malloc(SQUnsignedInteger size);
LAVRIL_API void *lv_realloc(void *p, SQUnsignedInteger oldsize, SQUnsignedInteger newsize);
LAVRIL_API void lv_free(void *p, SQUnsignedInteger size);

/* Debug */
LAVRIL_API SQRESULT lv_stackinfos(HSQUIRRELVM v, SQInteger level, SQStackInfos *si);
LAVRIL_API void lv_setdebughook(HSQUIRRELVM v);
LAVRIL_API void lv_setnativedebughook(HSQUIRRELVM v, SQDEBUGHOOK hook);

/* IO */
LAVRIL_API LVFILE lv_fopen(const SQChar *, const SQChar *);
LAVRIL_API SQInteger lv_fread(SQUserPointer, SQInteger, SQInteger, LVFILE);
LAVRIL_API SQInteger lv_fwrite(const SQUserPointer, SQInteger, SQInteger, LVFILE);
LAVRIL_API SQInteger lv_fseek(LVFILE , SQInteger , SQInteger);
LAVRIL_API SQInteger lv_ftell(LVFILE);
LAVRIL_API SQInteger lv_fflush(LVFILE);
LAVRIL_API SQInteger lv_fclose(LVFILE);
LAVRIL_API SQInteger lv_feof(LVFILE);
LAVRIL_API SQRESULT lv_createfile(HSQUIRRELVM v, LVFILE file, SQBool own);
LAVRIL_API SQRESULT lv_getfile(HSQUIRRELVM v, SQInteger idx, LVFILE *file);

/* IO compiler helpers */
LAVRIL_API SQRESULT lv_loadfile(HSQUIRRELVM v, const SQChar *filename, SQBool printerror);
LAVRIL_API SQRESULT lv_execfile(HSQUIRRELVM v, const SQChar *filename, SQBool retval, SQBool printerror);
LAVRIL_API SQRESULT lv_writeclosuretofile(HSQUIRRELVM v, const SQChar *filename);

/* Blob */
LAVRIL_API SQUserPointer lv_createblob(HSQUIRRELVM v, SQInteger size);
LAVRIL_API SQRESULT lv_getblob(HSQUIRRELVM v, SQInteger idx, SQUserPointer *ptr);
LAVRIL_API SQInteger lv_getblobsize(HSQUIRRELVM v, SQInteger idx);

/* String */
#ifdef REGEX
LAVRIL_API SQRex *sqstd_rex_compile(const SQChar *pattern, const SQChar **error);
LAVRIL_API void sqstd_rex_free(SQRex *exp);
LAVRIL_API SQBool sqstd_rex_match(SQRex *exp, const SQChar *text);
LAVRIL_API SQBool sqstd_rex_search(SQRex *exp, const SQChar *text, const SQChar **out_begin, const SQChar **out_end);
LAVRIL_API SQBool sqstd_rex_searchrange(SQRex *exp, const SQChar *text_begin, const SQChar *text_end, const SQChar **out_begin, const SQChar **out_end);
LAVRIL_API SQInteger sqstd_rex_getsubexpcount(SQRex *exp);
LAVRIL_API SQBool sqstd_rex_getsubexp(SQRex *exp, SQInteger n, SQRexMatch *subexp);
#endif
LAVRIL_API SQRESULT sqstd_format(HSQUIRRELVM v, SQInteger nformatstringidx, SQInteger *outlen, SQChar **output);

/* Modules */
LAVRIL_API SQRESULT mod_init_io(HSQUIRRELVM v);
LAVRIL_API SQRESULT mod_init_blob(HSQUIRRELVM v);
LAVRIL_API SQRESULT mod_init_string(HSQUIRRELVM v);

#include "modules.h"

/* Auxiliary macros */
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
# define LV_UNUSED_ARG(x) __attribute__((unused)) x
#else
# define LV_UNUSED_ARG(x) ((void)x)
#endif

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*_LAVRIL_H_*/
