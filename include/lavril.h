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

struct LVVM;
struct LVTable;
struct LVArray;
struct LVString;
struct LVClosure;
struct LVGenerator;
struct LVNativeClosure;
struct LVUserData;
struct LVFunctionProto; //TODO unused
struct LVRefCounted;
struct LVClass;
struct LVInstance;
struct LVDelegable;
struct LVOuter;

#ifdef _UNICODE
#define LVUNICODE
#endif

#include "config.h"

#define LAVRIL_VERSION    _LC("Lavril 1.3-beta")
#define LAVRIL_COPYRIGHT  _LC("Copyright (C) 2015-2016 Mavicona, Quenza Inc.\nAll Rights Reserved")
#define LAVRIL_AUTHOR     _LC("Quenza Inc.")
#define LAVRIL_VERSION_NUMBER 130

#define LV_VMSTATE_IDLE         0
#define LV_VMSTATE_RUNNING      1
#define LV_VMSTATE_SUSPENDED    2

#define LAVRIL_EOB            0

#define BYTECODE_STREAM_TAG   0xBEBE

#define OBJECT_REF_COUNTED    0x08000000
#define OBJECT_NUMERIC        0x04000000
#define OBJECT_DELEGABLE      0x02000000
#define OBJECT_CANBEFALSE     0x01000000

#define LV_MATCHTYPEMASKSTRING (-99999)

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

#define STREAM_TYPE_TAG 0x80000000

#define init_module(mod,v) mod_init_##mod(v)
#define register_module(mod) LAVRIL_API LVRESULT mod_init_##mod(VMHANDLE v)

/* File operation modes */
#define LV_SEEK_CUR 0
#define LV_SEEK_END 1
#define LV_SEEK_SET 2

typedef enum {
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
} LVObjectType;

#define ISREFCOUNTED(t) (t&OBJECT_REF_COUNTED)

typedef union {
	struct LVTable *pTable;
	struct LVArray *pArray;
	struct LVClosure *pClosure;
	struct LVOuter *pOuter;
	struct LVGenerator *pGenerator;
	struct LVNativeClosure *pNativeClosure;
	struct LVString *pString;
	struct LVUserData *pUserData;
	LVInteger nInteger;
	LVFloat fFloat;
	LVUserPointer pUserPointer;
	struct FunctionPrototype *pFunctionProto;
	struct LVRefCounted *pRefCounted;
	struct LVDelegable *pDelegable;
	struct LVVM *pThread;
	struct LVClass *pClass;
	struct LVInstance *pInstance;
	struct LVWeakRef *pWeakRef;
	LVRawObjectVal raw;
} LVObjectValue;

typedef struct {
	LVObjectType _type;
	LVObjectValue _unVal;
} LVObject;

//TODO not used
typedef struct {
	LVBool _static;
	LVInteger _index;
} LVMemberHandle;

typedef struct {
	const LVChar *funcname;
	const LVChar *source;
	LVInteger line;
} LVStackInfos;

typedef struct LVVM *VMHANDLE;
typedef LVObject OBJHANDLE;
typedef LVMemberHandle MEMBERHANDLE;
typedef LVInteger (*LVFUNCTION)(VMHANDLE);
typedef LVInteger (*LVRELEASEHOOK)(LVUserPointer, LVInteger size);
typedef CALLBACK void (*LVCOMPILERERROR)(VMHANDLE, const LVChar * /*desc*/, const LVChar * /*source*/, LVInteger /*line*/, LVInteger /*column*/);
typedef CALLBACK void (*LVPRINTFUNCTION)(VMHANDLE, const LVChar *, ...);
typedef CALLBACK void (*LVDEBUGHOOK)(VMHANDLE /*v*/, LVInteger /*type*/, const LVChar * /*sourcename*/, LVInteger /*line*/, const LVChar * /*funcname*/);
typedef LVInteger (*LVLOADUNIT)(VMHANDLE /*v*/, const LVChar * /*sourcename*/, LVBool /*printerr*/);
typedef LVInteger (*LVWRITEFUNC)(LVUserPointer, LVUserPointer, LVInteger);
typedef LVInteger (*LVREADFUNC)(LVUserPointer, LVUserPointer, LVInteger);
typedef LVInteger (*LVLEXREADFUNC)(LVUserPointer);

typedef struct {
	const LVChar *name;
	LVFUNCTION f;
	LVInteger nparamscheck;
	const LVChar *typemask;
} LVRegFunction;

typedef struct {
	LVUserPointer funcid;
	const LVChar *name;
	const LVChar *source;
	LVInteger line;
} LVFunctionInfo;

typedef void *LVFILE;

/* VM */
LAVRIL_API VMHANDLE lv_open(LVInteger initialstacksize);
LAVRIL_API VMHANDLE lv_newthread(VMHANDLE friendvm, LVInteger initialstacksize);
LAVRIL_API void lv_seterrorhandler(VMHANDLE v);
LAVRIL_API void lv_close(VMHANDLE v);
LAVRIL_API void lv_setforeignptr(VMHANDLE v, LVUserPointer p);
LAVRIL_API LVUserPointer lv_getforeignptr(VMHANDLE v);
LAVRIL_API void lv_setsharedforeignptr(VMHANDLE v, LVUserPointer p);
LAVRIL_API LVUserPointer lv_getsharedforeignptr(VMHANDLE v);
LAVRIL_API void lv_setvmreleasehook(VMHANDLE v, LVRELEASEHOOK hook);
LAVRIL_API LVRELEASEHOOK lv_getvmreleasehook(VMHANDLE v);
LAVRIL_API void lv_setsharedreleasehook(VMHANDLE v, LVRELEASEHOOK hook);
LAVRIL_API LVRELEASEHOOK lv_getsharedreleasehook(VMHANDLE v);
LAVRIL_API void lv_setprintfunc(VMHANDLE v, LVPRINTFUNCTION printfunc, LVPRINTFUNCTION errfunc);
LAVRIL_API LVPRINTFUNCTION lv_getprintfunc(VMHANDLE v);
LAVRIL_API LVPRINTFUNCTION lv_geterrorfunc(VMHANDLE v);
LAVRIL_API LVRESULT lv_suspendvm(VMHANDLE v);
LAVRIL_API LVRESULT lv_wakeupvm(VMHANDLE v, LVBool resumedret, LVBool retval, LVBool raiseerror, LVBool throwerror);
LAVRIL_API LVInteger lv_getvmstate(VMHANDLE v);
LAVRIL_API LVInteger lv_getversion();

/* Compiler */
LAVRIL_API LVRESULT lv_compile(VMHANDLE v, LVLEXREADFUNC read, LVUserPointer p, const LVChar *sourcename, LVBool raiseerror);
LAVRIL_API LVRESULT lv_compilebuffer(VMHANDLE v, const LVChar *s, LVInteger size, const LVChar *sourcename, LVBool raiseerror);
LAVRIL_API void lv_enabledebuginfo(VMHANDLE v, LVBool enable);
LAVRIL_API void lv_notifyallexceptions(VMHANDLE v, LVBool enable);
LAVRIL_API void lv_setcompilererrorhandler(VMHANDLE v, LVCOMPILERERROR f);
LAVRIL_API void lv_setunitloader(VMHANDLE v, LVLOADUNIT f);

/* Stack operations */
LAVRIL_API void lv_push(VMHANDLE v, LVInteger idx);
LAVRIL_API void lv_pop(VMHANDLE v, LVInteger nelemstopop);
LAVRIL_API void lv_poptop(VMHANDLE v);
LAVRIL_API void lv_remove(VMHANDLE v, LVInteger idx);
LAVRIL_API LVInteger lv_gettop(VMHANDLE v);
LAVRIL_API void lv_settop(VMHANDLE v, LVInteger newtop);
LAVRIL_API LVRESULT lv_reservestack(VMHANDLE v, LVInteger nsize);
LAVRIL_API LVInteger lv_cmp(VMHANDLE v);
LAVRIL_API void lv_move(VMHANDLE dest, VMHANDLE src, LVInteger idx);
#ifdef _DEBUG
LAVRIL_API void lv_stackdump(VMHANDLE v);
#endif

/* Object creation handling */
LAVRIL_API LVUserPointer lv_newuserdata(VMHANDLE v, LVUnsignedInteger size);
LAVRIL_API void lv_newtable(VMHANDLE v);
LAVRIL_API void lv_newtableex(VMHANDLE v, LVInteger initialcapacity);
LAVRIL_API void lv_newarray(VMHANDLE v, LVInteger size);
LAVRIL_API void lv_newclosure(VMHANDLE v, LVFUNCTION func, LVUnsignedInteger nfreevars);
LAVRIL_API LVRESULT lv_setparamscheck(VMHANDLE v, LVInteger nparamscheck, const LVChar *typemask);
LAVRIL_API LVRESULT lv_bindenv(VMHANDLE v, LVInteger idx);
LAVRIL_API LVRESULT lv_setclosureroot(VMHANDLE v, LVInteger idx);
LAVRIL_API LVRESULT lv_getclosureroot(VMHANDLE v, LVInteger idx);
LAVRIL_API void lv_pushstring(VMHANDLE v, const LVChar *s, LVInteger len);
LAVRIL_API void lv_pushfloat(VMHANDLE v, LVFloat f);
LAVRIL_API void lv_pushinteger(VMHANDLE v, LVInteger n);
LAVRIL_API void lv_pushbool(VMHANDLE v, LVBool b);
LAVRIL_API void lv_pushuserpointer(VMHANDLE v, LVUserPointer p);
LAVRIL_API void lv_pushnull(VMHANDLE v);
LAVRIL_API void lv_pushthread(VMHANDLE v, VMHANDLE thread);
LAVRIL_API LVObjectType lv_gettype(VMHANDLE v, LVInteger idx);
LAVRIL_API LVRESULT lv_typeof(VMHANDLE v, LVInteger idx);
LAVRIL_API LVInteger lv_getsize(VMHANDLE v, LVInteger idx);
LAVRIL_API LVHash lv_gethash(VMHANDLE v, LVInteger idx);
LAVRIL_API LVRESULT lv_getbase(VMHANDLE v, LVInteger idx);
LAVRIL_API LVBool lv_instanceof(VMHANDLE v);
LAVRIL_API LVRESULT lv_tostring(VMHANDLE v, LVInteger idx);
LAVRIL_API void lv_tobool(VMHANDLE v, LVInteger idx, LVBool *b);
LAVRIL_API LVRESULT lv_getstring(VMHANDLE v, LVInteger idx, const LVChar **c);
LAVRIL_API LVRESULT lv_getinteger(VMHANDLE v, LVInteger idx, LVInteger *i);
LAVRIL_API LVRESULT lv_getfloat(VMHANDLE v, LVInteger idx, LVFloat *f);
LAVRIL_API LVRESULT lv_getbool(VMHANDLE v, LVInteger idx, LVBool *b);
LAVRIL_API LVRESULT lv_getthread(VMHANDLE v, LVInteger idx, VMHANDLE *thread);
LAVRIL_API LVRESULT lv_getuserpointer(VMHANDLE v, LVInteger idx, LVUserPointer *p);
LAVRIL_API LVRESULT lv_getuserdata(VMHANDLE v, LVInteger idx, LVUserPointer *p, LVUserPointer *typetag);
LAVRIL_API LVRESULT lv_settypetag(VMHANDLE v, LVInteger idx, LVUserPointer typetag);
LAVRIL_API LVRESULT lv_gettypetag(VMHANDLE v, LVInteger idx, LVUserPointer *typetag);
LAVRIL_API void lv_setreleasehook(VMHANDLE v, LVInteger idx, LVRELEASEHOOK hook);
LAVRIL_API LVRELEASEHOOK lv_getreleasehook(VMHANDLE v, LVInteger idx);
LAVRIL_API LVChar *lv_getscratchpad(VMHANDLE v, LVInteger minsize);
LAVRIL_API LVRESULT lv_getfunctioninfo(VMHANDLE v, LVInteger level, LVFunctionInfo *fi);
LAVRIL_API LVRESULT lv_getclosureinfo(VMHANDLE v, LVInteger idx, LVUnsignedInteger *nparams, LVUnsignedInteger *nfreevars);
LAVRIL_API LVRESULT lv_getclosurename(VMHANDLE v, LVInteger idx);
LAVRIL_API LVRESULT lv_setnativeclosurename(VMHANDLE v, LVInteger idx, const LVChar *name);
LAVRIL_API LVRESULT lv_setinstanceup(VMHANDLE v, LVInteger idx, LVUserPointer p);
LAVRIL_API LVRESULT lv_getinstanceup(VMHANDLE v, LVInteger idx, LVUserPointer *p, LVUserPointer typetag);
LAVRIL_API LVRESULT lv_setclassudsize(VMHANDLE v, LVInteger idx, LVInteger udsize);
LAVRIL_API LVRESULT lv_newclass(VMHANDLE v, LVBool hasbase);
LAVRIL_API LVRESULT lv_createinstance(VMHANDLE v, LVInteger idx);
LAVRIL_API LVRESULT lv_setattributes(VMHANDLE v, LVInteger idx);
LAVRIL_API LVRESULT lv_getattributes(VMHANDLE v, LVInteger idx);
LAVRIL_API LVRESULT lv_getclass(VMHANDLE v, LVInteger idx);
LAVRIL_API void lv_weakref(VMHANDLE v, LVInteger idx);
LAVRIL_API LVRESULT lv_getdefaultdelegate(VMHANDLE v, LVObjectType t);
LAVRIL_API LVRESULT lv_getmemberhandle(VMHANDLE v, LVInteger idx, MEMBERHANDLE *handle);
LAVRIL_API LVRESULT lv_getbyhandle(VMHANDLE v, LVInteger idx, const MEMBERHANDLE *handle);
LAVRIL_API LVRESULT lv_setbyhandle(VMHANDLE v, LVInteger idx, const MEMBERHANDLE *handle);

/* Object manipulation */
LAVRIL_API void lv_pushroottable(VMHANDLE v);
LAVRIL_API void lv_pushregistrytable(VMHANDLE v);
LAVRIL_API void lv_pushconsttable(VMHANDLE v);
LAVRIL_API LVRESULT lv_setroottable(VMHANDLE v);
LAVRIL_API LVRESULT lv_setconsttable(VMHANDLE v);
LAVRIL_API LVRESULT lv_newslot(VMHANDLE v, LVInteger idx, LVBool bstatic);
LAVRIL_API LVRESULT lv_deleteslot(VMHANDLE v, LVInteger idx, LVBool pushval);
LAVRIL_API LVRESULT lv_set(VMHANDLE v, LVInteger idx);
LAVRIL_API LVRESULT lv_get(VMHANDLE v, LVInteger idx);
LAVRIL_API LVRESULT lv_rawget(VMHANDLE v, LVInteger idx);
LAVRIL_API LVRESULT lv_rawset(VMHANDLE v, LVInteger idx);
LAVRIL_API LVRESULT lv_rawdeleteslot(VMHANDLE v, LVInteger idx, LVBool pushval);
LAVRIL_API LVRESULT lv_newmember(VMHANDLE v, LVInteger idx, LVBool bstatic);
LAVRIL_API LVRESULT lv_rawnewmember(VMHANDLE v, LVInteger idx, LVBool bstatic);
LAVRIL_API LVRESULT lv_arrayappend(VMHANDLE v, LVInteger idx);
LAVRIL_API LVRESULT lv_arraypop(VMHANDLE v, LVInteger idx, LVBool pushval);
LAVRIL_API LVRESULT lv_arrayresize(VMHANDLE v, LVInteger idx, LVInteger newsize);
LAVRIL_API LVRESULT lv_arrayreverse(VMHANDLE v, LVInteger idx);
LAVRIL_API LVRESULT lv_arrayremove(VMHANDLE v, LVInteger idx, LVInteger itemidx);
LAVRIL_API LVRESULT lv_arrayinsert(VMHANDLE v, LVInteger idx, LVInteger destpos);
LAVRIL_API LVRESULT lv_setdelegate(VMHANDLE v, LVInteger idx);
LAVRIL_API LVRESULT lv_getdelegate(VMHANDLE v, LVInteger idx);
LAVRIL_API LVRESULT lv_clone(VMHANDLE v, LVInteger idx);
LAVRIL_API LVRESULT lv_setfreevariable(VMHANDLE v, LVInteger idx, LVUnsignedInteger nval);
LAVRIL_API LVRESULT lv_next(VMHANDLE v, LVInteger idx);
LAVRIL_API LVRESULT lv_getweakrefval(VMHANDLE v, LVInteger idx);
LAVRIL_API LVRESULT lv_clear(VMHANDLE v, LVInteger idx);

/* Calls */
LAVRIL_API LVRESULT lv_call(VMHANDLE v, LVInteger params, LVBool retval, LVBool raiseerror);
LAVRIL_API LVRESULT lv_resume(VMHANDLE v, LVBool retval, LVBool raiseerror);
LAVRIL_API const LVChar *lv_getlocal(VMHANDLE v, LVUnsignedInteger level, LVUnsignedInteger idx);
LAVRIL_API LVRESULT lv_getcallee(VMHANDLE v);
LAVRIL_API const LVChar *lv_getfreevariable(VMHANDLE v, LVInteger idx, LVUnsignedInteger nval);
LAVRIL_API LVRESULT lv_throwerror(VMHANDLE v, const LVChar *err);
LAVRIL_API LVRESULT lv_throwobject(VMHANDLE v);
LAVRIL_API void lv_reseterror(VMHANDLE v);
LAVRIL_API void lv_getlasterror(VMHANDLE v);
LAVRIL_API void lv_registererrorhandlers(VMHANDLE v);

/* Raw object handling */
LAVRIL_API LVRESULT lv_getstackobj(VMHANDLE v, LVInteger idx, OBJHANDLE *po);
LAVRIL_API void lv_pushobject(VMHANDLE v, OBJHANDLE obj);
LAVRIL_API void lv_addref(VMHANDLE v, OBJHANDLE *po);
LAVRIL_API LVBool lv_release(VMHANDLE v, OBJHANDLE *po);
LAVRIL_API LVUnsignedInteger lv_getrefcount(VMHANDLE v, OBJHANDLE *po);
LAVRIL_API void lv_resetobject(OBJHANDLE *po);
LAVRIL_API const LVChar *lv_objtostring(const OBJHANDLE *o);
LAVRIL_API LVBool lv_objtobool(const OBJHANDLE *o);
LAVRIL_API LVInteger lv_objtointeger(const OBJHANDLE *o);
LAVRIL_API LVFloat lv_objtofloat(const OBJHANDLE *o);
LAVRIL_API LVUserPointer lv_objtouserpointer(const OBJHANDLE *o);
LAVRIL_API LVRESULT lv_getobjtypetag(const OBJHANDLE *o, LVUserPointer *typetag);
LAVRIL_API LVUnsignedInteger lv_getvmrefcount(VMHANDLE v, const OBJHANDLE *po);

/* GC */
LAVRIL_API LVInteger lv_collectgarbage(VMHANDLE v);
LAVRIL_API LVRESULT lv_resurrectunreachable(VMHANDLE v);

/* Serialization */
LAVRIL_API LVRESULT lv_writeclosure(VMHANDLE vm, LVWRITEFUNC writef, LVUserPointer up);
LAVRIL_API LVRESULT lv_readclosure(VMHANDLE vm, LVREADFUNC readf, LVUserPointer up);

/* Memory */
LAVRIL_API void *lv_malloc(LVUnsignedInteger size);
LAVRIL_API void *lv_realloc(void *p, LVUnsignedInteger oldsize, LVUnsignedInteger newsize);
LAVRIL_API void lv_free(void *p, LVUnsignedInteger size);

/* Debug */
LAVRIL_API LVRESULT lv_stackinfos(VMHANDLE v, LVInteger level, LVStackInfos *si);
LAVRIL_API void lv_setdebughook(VMHANDLE v);
LAVRIL_API void lv_setnativedebughook(VMHANDLE v, LVDEBUGHOOK hook);

/* IO */
LAVRIL_API LVFILE lv_fopen(const LVChar *, const LVChar *);
LAVRIL_API LVInteger lv_fread(LVUserPointer, LVInteger, LVInteger, LVFILE);
LAVRIL_API LVInteger lv_fwrite(const LVUserPointer, LVInteger, LVInteger, LVFILE);
LAVRIL_API LVInteger lv_fseek(LVFILE , LVInteger , LVInteger);
LAVRIL_API LVInteger lv_ftell(LVFILE);
LAVRIL_API LVInteger lv_fflush(LVFILE);
LAVRIL_API LVInteger lv_fclose(LVFILE);
LAVRIL_API LVInteger lv_feof(LVFILE);
LAVRIL_API LVRESULT lv_createfile(VMHANDLE v, LVFILE file, LVBool own);
LAVRIL_API LVRESULT lv_getfile(VMHANDLE v, LVInteger idx, LVFILE *file);

/* IO compiler helpers */
LAVRIL_API LVRESULT lv_loadfile(VMHANDLE v, const LVChar *filename, LVBool printerror);
LAVRIL_API LVRESULT lv_execfile(VMHANDLE v, const LVChar *filename, LVBool retval, LVBool printerror);
LAVRIL_API LVRESULT lv_writeclosuretofile(VMHANDLE v, const LVChar *filename);

/* Blob */
LAVRIL_API LVUserPointer lv_createblob(VMHANDLE v, LVInteger size);
LAVRIL_API LVRESULT lv_getblob(VMHANDLE v, LVInteger idx, LVUserPointer *ptr);
LAVRIL_API LVInteger lv_getblobsize(VMHANDLE v, LVInteger idx);

/* String */
#ifdef REGEX
LAVRIL_API LVRex *sqstd_rex_compile(const LVChar *pattern, const LVChar **error);
LAVRIL_API void sqstd_rex_free(LVRex *exp);
LAVRIL_API LVBool sqstd_rex_match(LVRex *exp, const LVChar *text);
LAVRIL_API LVBool sqstd_rex_search(LVRex *exp, const LVChar *text, const LVChar **out_begin, const LVChar **out_end);
LAVRIL_API LVBool sqstd_rex_searchrange(LVRex *exp, const LVChar *text_begin, const LVChar *text_end, const LVChar **out_begin, const LVChar **out_end);
LAVRIL_API LVInteger sqstd_rex_getsubexpcount(LVRex *exp);
LAVRIL_API LVBool sqstd_rex_getsubexp(LVRex *exp, LVInteger n, LVRexMatch *subexp);
#endif
LAVRIL_API LVRESULT sqstd_format(VMHANDLE v, LVInteger nformatstringidx, LVInteger *outlen, LVChar **output);

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
