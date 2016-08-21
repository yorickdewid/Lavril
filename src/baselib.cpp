#include "pcheader.h"
#include "vm.h"
#include "lvstring.h"
#include "table.h"
#include "array.h"
#include "funcproto.h"
#include "closure.h"
#include "class.h"

#include <stdarg.h>
#include <ctype.h>

static bool str2num(const LVChar *s, LVObjectPtr& res, LVInteger base) {
	LVChar *end;
	const LVChar *e = s;
	bool iseintbase = base > 13; //to fix error converting hexadecimals with e like 56f0791e
	bool isfloat = false;
	LVChar c;
	while ((c = *e) != _LC('\0')) {
		if (c == _LC('.') || (!iseintbase && (c == _LC('E') || c == _LC('e')))) { //e and E is for scientific notation
			isfloat = true;
			break;
		}
		e++;
	}
	if (isfloat) {
		LVFloat r = LVFloat(scstrtod(s, &end));
		if (s == end) return false;
		res = r;
	} else {
		LVInteger r = LVInteger(scstrtol(s, &end, (int)base));
		if (s == end) return false;
		res = r;
	}
	return true;
}

#define MAX_FORMAT_LEN  20
#define MAX_WFORMAT_LEN 3
#define ADDITIONAL_FORMAT_SPACE (100*sizeof(LVChar))

static LVBool isfmtchr(LVChar ch) {
	switch (ch) {
		case '-':
		case '+':
		case ' ':
		case '#':
		case '0':
			return LVTrue;
	}
	return LVFalse;
}

static LVInteger validate_format(VMHANDLE v, LVChar *fmt, const LVChar *src, LVInteger n, LVInteger& width) {
	LVChar *dummy;
	LVChar swidth[MAX_WFORMAT_LEN];
	LVInteger wc = 0;
	LVInteger start = n;
	fmt[0] = '%';

	while (isfmtchr(src[n]))
		n++;
	while (scisdigit(src[n])) {
		swidth[wc] = src[n];
		n++;
		wc++;
		if (wc >= MAX_WFORMAT_LEN)
			return lv_throwerror(v, _LC("width format too long"));
	}
	swidth[wc] = '\0';
	if (wc > 0) {
		width = scstrtol(swidth, &dummy, 10);
	} else
		width = 0;
	if (src[n] == '.') {
		n++;

		wc = 0;
		while (scisdigit(src[n])) {
			swidth[wc] = src[n];
			n++;
			wc++;
			if (wc >= MAX_WFORMAT_LEN)
				return lv_throwerror(v, _LC("precision format too long"));
		}
		swidth[wc] = '\0';
		if (wc > 0) {
			width += scstrtol(swidth, &dummy, 10);

		}
	}
	if (n - start > MAX_FORMAT_LEN )
		return lv_throwerror(v, _LC("format too long"));
	memcpy(&fmt[1], &src[start], ((n - start) + 1)*sizeof(LVChar));
	fmt[(n - start) + 2] = '\0';
	return n;
}

LVRESULT strformat(VMHANDLE v, LVInteger nformatstringidx, LVInteger *outlen, LVChar **output) {
	const LVChar *format;
	LVChar *dest;
	LVChar fmt[MAX_FORMAT_LEN];
	lv_getstring(v, nformatstringidx, &format);
	LVInteger format_size = lv_getsize(v, nformatstringidx);
	LVInteger allocated = (format_size + 2) * sizeof(LVChar);
	dest = lv_getscratchpad(v, allocated);
	LVInteger n = 0, i = 0, nparam = nformatstringidx + 1, w = 0;

	//while(format[n] != '\0')
	while (n < format_size) {
		if (format[n] != '%') {
			assert(i < allocated);
			dest[i++] = format[n];
			n++;
		} else if (format[n + 1] == '%') { //handles %%
			dest[i++] = '%';
			n += 2;
		} else {
			n++;
			if (nparam > lv_gettop(v))
				return lv_throwerror(v, _LC("not enough paramters for the given format string"));
			n = validate_format(v, fmt, format, n, w);
			if (n < 0)
				return -1;
			LVInteger addlen = 0;
			LVInteger valtype = 0;
			const LVChar *ts = NULL;
			LVInteger ti = 0;
			LVFloat tf = 0;
			switch (format[n]) {
				case 's':
					if (LV_FAILED(lv_getstring(v, nparam, &ts)))
						return lv_throwerror(v, _LC("string expected for the specified format"));
					addlen = (lv_getsize(v, nparam) * sizeof(LVChar)) + ((w + 1) * sizeof(LVChar));
					valtype = 's';
					break;
				case 'i':
				case 'd':
				case 'o':
				case 'u':
				case 'x':
				case 'X':
#ifdef _LV64
				{
					size_t flen = scstrlen(fmt);
					LVInteger fpos = flen - 1;
					LVChar f = fmt[fpos];
					const LVChar *prec = (const LVChar *)_PRINT_INT_PREC;
					while (*prec != _LC('\0')) {
						fmt[fpos++] = *prec++;
					}
					fmt[fpos++] = f;
					fmt[fpos++] = _LC('\0');
				}
#endif
				case 'c':
					if (LV_FAILED(lv_getinteger(v, nparam, &ti)))
						return lv_throwerror(v, _LC("integer expected for the specified format"));
					addlen = (ADDITIONAL_FORMAT_SPACE) + ((w + 1) * sizeof(LVChar));
					valtype = 'i';
					break;
				case 'f':
				case 'g':
				case 'G':
				case 'e':
				case 'E':
					if (LV_FAILED(lv_getfloat(v, nparam, &tf)))
						return lv_throwerror(v, _LC("float expected for the specified format"));
					addlen = (ADDITIONAL_FORMAT_SPACE) + ((w + 1) * sizeof(LVChar));
					valtype = 'f';
					break;
				default:
					return lv_throwerror(v, _LC("invalid format"));
			}
			n++;
			allocated += addlen + sizeof(LVChar);
			dest = lv_getscratchpad(v, allocated);
			switch (valtype) {
				case 's':
					i += scsprintf(&dest[i], allocated, fmt, ts);
					break;
				case 'i':
					i += scsprintf(&dest[i], allocated, fmt, ti);
					break;
				case 'f':
					i += scsprintf(&dest[i], allocated, fmt, tf);
					break;
			};
			nparam ++;
		}
	}
	*outlen = i;
	dest[i] = '\0';
	*output = dest;
	return LV_OK;
}

static LVInteger base_blub(VMHANDLE LV_UNUSED_ARG(v)) {
	return 0;
}

#ifndef NO_GARBAGE_COLLECTOR
static LVInteger base_collectgarbage(VMHANDLE v) {
	lv_pushinteger(v, lv_collectgarbage(v));
	return 1;
}
static LVInteger base_resurectureachable(VMHANDLE v) {
	lv_resurrectunreachable(v);
	return 1;
}
#endif

static LVInteger base_getroottable(VMHANDLE v) {
	v->Push(v->_roottable);
	return 1;
}

static LVInteger base_getconsttable(VMHANDLE v) {
	v->Push(_ss(v)->_consts);
	return 1;
}

static LVInteger base_setroottable(VMHANDLE v) {
	LVObjectPtr o = v->_roottable;
	if (LV_FAILED(lv_setroottable(v)))
		return LV_ERROR;
	v->Push(o);
	return 1;
}

static LVInteger base_setconsttable(VMHANDLE v) {
	LVObjectPtr o = _ss(v)->_consts;
	if (LV_FAILED(lv_setconsttable(v)))
		return LV_ERROR;
	v->Push(o);
	return 1;
}

static LVInteger base_seterrorhandler(VMHANDLE v) {
	lv_seterrorhandler(v);
	return 0;
}

static LVInteger base_setdebughook(VMHANDLE v) {
	lv_setdebughook(v);
	return 0;
}

static LVInteger base_enabledebuginfo(VMHANDLE v) {
	LVObjectPtr& o = stack_get(v, 2);

	lv_enabledebuginfo(v, LVVM::IsFalse(o) ? LVFalse : LVTrue);
	return 0;
}

static LVInteger __getcallstackinfos(VMHANDLE v, LVInteger level) {
	LVStackInfos si;
	LVInteger seq = 0;
	const LVChar *name = NULL;

	if (LV_SUCCEEDED(lv_stackinfos(v, level, &si))) {
		const LVChar *fn = _LC("unknown");
		const LVChar *src = _LC("unknown");
		if (si.funcname)
			fn = si.funcname;
		if (si.source)
			src = si.source;
		lv_newtable(v);
		lv_pushstring(v, _LC("func"), -1);
		lv_pushstring(v, fn, -1);
		lv_newslot(v, -3, LVFalse);
		lv_pushstring(v, _LC("src"), -1);
		lv_pushstring(v, src, -1);
		lv_newslot(v, -3, LVFalse);
		lv_pushstring(v, _LC("line"), -1);
		lv_pushinteger(v, si.line);
		lv_newslot(v, -3, LVFalse);
		lv_pushstring(v, _LC("variables"), -1);
		lv_newtable(v);
		seq = 0;
		while ((name = lv_getlocal(v, level, seq))) {
			lv_pushstring(v, name, -1);
			lv_push(v, -2);
			lv_newslot(v, -4, LVFalse);
			lv_pop(v, 1);
			seq++;
		}
		lv_newslot(v, -3, LVFalse);
		return 1;
	}

	return 0;
}
static LVInteger base_getstackinfos(VMHANDLE v) {
	LVInteger level;
	lv_getinteger(v, -1, &level);
	return __getcallstackinfos(v, level);
}

static LVInteger base_assert(VMHANDLE v) {
	if (LVVM::IsFalse(stack_get(v, 2))) {
		return lv_throwerror(v, _LC("assertion failed"));
	}
	return 0;
}

static LVInteger get_slice_params(VMHANDLE v, LVInteger& sidx, LVInteger& eidx, LVObjectPtr& o) {
	LVInteger top = lv_gettop(v);
	sidx = 0;
	eidx = 0;
	o = stack_get(v, 1);
	if (top > 1) {
		LVObjectPtr& start = stack_get(v, 2);
		if (type(start) != OT_NULL && lv_isnumeric(start)) {
			sidx = tointeger(start);
		}
	}
	if (top > 2) {
		LVObjectPtr& end = stack_get(v, 3);
		if (lv_isnumeric(end)) {
			eidx = tointeger(end);
		}
	} else {
		eidx = lv_getsize(v, 1);
	}
	return 1;
}

static LVInteger base_readline(VMHANDLE v) {
	LVChar *dest;
	if (_ss(v)->_readfunc) {
		dest = _ss(v)->_readfunc(v);
		lv_pushstring(v, dest, -1);
		lv_free(dest, 0);
		return 1;
	}
	return 0;
}

static LVInteger base_print(VMHANDLE v) {
	const LVChar *str;
	if (LV_SUCCEEDED(lv_tostring(v, 2))) {
		if (LV_SUCCEEDED(lv_getstring(v, -1, &str))) {
			if (_ss(v)->_printfunc)
				_ss(v)->_printfunc(v, _LC("%s"), str);
			return 0;
		}
	}
	return LV_ERROR;
}

static LVInteger base_printf(VMHANDLE v) {
	LVChar *dest;
	LVInteger length = 0;
	if (LV_FAILED(strformat(v, 2, &length, &dest)))
		return LV_ERROR;
	if (_ss(v)->_printfunc)
		_ss(v)->_printfunc(v, _LC("%s"), dest);
	return 0;
}

static LVInteger base_println(VMHANDLE v) {
	const LVChar *str;
	if (LV_SUCCEEDED(lv_tostring(v, 2))) {
		if (LV_SUCCEEDED(lv_getstring(v, -1, &str))) {
			if (_ss(v)->_printfunc)
				_ss(v)->_printfunc(v, _LC("%s\n"), str);
			return 0;
		}
	}
	return LV_ERROR;
}

static LVInteger base_error(VMHANDLE v) {
	const LVChar *str;
	if (LV_SUCCEEDED(lv_tostring(v, 2))) {
		if (LV_SUCCEEDED(lv_getstring(v, -1, &str))) {
			if (_ss(v)->_errorfunc)
				_ss(v)->_errorfunc(v, _LC("%s"), str);
			return 0;
		}
	}
	return LV_ERROR;
}

static LVInteger base_compilestring(VMHANDLE v) {
	LVInteger nargs = lv_gettop(v);
	const LVChar *src = NULL, *name = _LC("unnamedbuffer");
	LVInteger size;
	lv_getstring(v, 2, &src);
	size = lv_getsize(v, 2);
	if (nargs > 2) {
		lv_getstring(v, 3, &name);
	}
	if (LV_SUCCEEDED(lv_compilebuffer(v, src, size, name, LVFalse)))
		return 1;
	else
		return LV_ERROR;
}

static LVInteger base_newthread(VMHANDLE v) {
	LVObjectPtr& func = stack_get(v, 2);
	LVInteger stksize = (_closure(func)->_function->_stacksize << 1) + 2;
	VMHANDLE newv = lv_newthread(v, (stksize < MIN_STACK_OVERHEAD + 2) ? MIN_STACK_OVERHEAD + 2 : stksize);
	lv_move(newv, v, -2);
	return 1;
}

static LVInteger base_suspend(VMHANDLE v) {
	return lv_suspendvm(v);
}

static LVInteger base_array(VMHANDLE v) {
	LVArray *a;
	LVObject& size = stack_get(v, 2);
	if (lv_gettop(v) > 2) {
		a = LVArray::Create(_ss(v), 0);
		a->Resize(tointeger(size), stack_get(v, 3));
	} else {
		a = LVArray::Create(_ss(v), tointeger(size));
	}
	v->Push(a);
	return 1;
}

static LVInteger base_type(VMHANDLE v) {
	LVObjectPtr& o = stack_get(v, 2);
	v->Push(LVString::Create(_ss(v), GetTypeName(o), -1));
	return 1;
}

static LVInteger base_callee(VMHANDLE v) {
	if (v->_callsstacksize > 1) {
		v->Push(v->_callsstack[v->_callsstacksize - 2]._closure);
		return 1;
	}
	return lv_throwerror(v, _LC("no closure in the calls stack"));
}

static const LVRegFunction base_funcs[] = {
	{_LC("seterrorhandler"), base_seterrorhandler, 2, NULL},
	{_LC("setdebughook"), base_setdebughook, 2, NULL},
	{_LC("enabledebuginfo"), base_enabledebuginfo, 2, NULL},
	{_LC("getstackinfos"), base_getstackinfos, 2, _LC(".n")},
	{_LC("getroottable"), base_getroottable, 1, NULL},
	{_LC("setroottable"), base_setroottable, 2, NULL},
	{_LC("getconsttable"), base_getconsttable, 1, NULL},
	{_LC("setconsttable"), base_setconsttable, 2, NULL},
	{_LC("assert"), base_assert, 2, NULL},
	{_LC("readline"), base_readline, 1, NULL},
	{_LC("echo"), base_print, 2, NULL},
	{_LC("print"), base_print, 2, NULL},
	{_LC("printf"), base_printf, -2, ".s"},
	{_LC("println"), base_println, 2, NULL},
	{_LC("puts"), base_println, 2, NULL},
	{_LC("error"), base_error, 2, NULL},
	{_LC("compilestring"), base_compilestring, -2, _LC(".ss")},
	{_LC("newthread"), base_newthread, 2, _LC(".c")},
	{_LC("suspend"), base_suspend, -1, NULL},
	{_LC("array"), base_array, -2, _LC(".n")},
	{_LC("type"), base_type, 2, NULL},
	{_LC("callee"), base_callee, 0, NULL},
	{_LC("blub"), base_blub, 0, NULL},
#ifndef NO_GARBAGE_COLLECTOR
	{_LC("collectgarbage"), base_collectgarbage, 0, NULL},
	{_LC("resurrectunreachable"), base_resurectureachable, 0, NULL},
#endif
	{NULL, (LVFUNCTION)0, 0, NULL}
};

void lv_base_register(VMHANDLE v) {
	LVInteger i = 0;

	lv_pushroottable(v);
	while (base_funcs[i].name != 0) {
		lv_pushstring(v, base_funcs[i].name, -1);
		lv_newclosure(v, base_funcs[i].f, 0);
		lv_setnativeclosurename(v, -1, base_funcs[i].name);
		lv_setparamscheck(v, base_funcs[i].nparamscheck, base_funcs[i].typemask);
		lv_newslot(v, -3, LVFalse);
		i++;
	}

	/* Additional modules */
	mod_init_io(v);
	mod_init_blob(v);
	mod_init_string(v);

	/* Global variables */
	lv_pushstring(v, _LC("_versionnumber_"), -1);
	lv_pushinteger(v, LAVRIL_VERSION_NUMBER);
	lv_newslot(v, -3, LVFalse);
	lv_pushstring(v, _LC("_version_"), -1);
	lv_pushstring(v, LAVRIL_VERSION, -1);
	lv_newslot(v, -3, LVFalse);
	lv_pushstring(v, _LC("_charsize_"), -1);
	lv_pushinteger(v, sizeof(LVChar));
	lv_newslot(v, -3, LVFalse);
	lv_pushstring(v, _LC("_intsize_"), -1);
	lv_pushinteger(v, sizeof(LVInteger));
	lv_newslot(v, -3, LVFalse);
	lv_pushstring(v, _LC("_floatsize_"), -1);
	lv_pushinteger(v, sizeof(LVFloat));
	lv_newslot(v, -3, LVFalse);
	lv_pushstring(v, _LC("_debug_"), -1);
#ifdef _DEBUG
	lv_pushbool(v, LVTrue);
#else
	lv_pushbool(v, LVFalse);
#endif
	lv_newslot(v, -3, LVFalse);
	lv_pop(v, 1);
}

static LVInteger default_delegate_len(VMHANDLE v) {
	v->Push(LVInteger(lv_getsize(v, 1)));
	return 1;
}

static LVInteger default_delegate_tofloat(VMHANDLE v) {
	LVObjectPtr& o = stack_get(v, 1);
	switch (type(o)) {
		case OT_STRING: {
			LVObjectPtr res;
			if (str2num(_stringval(o), res, 10)) {
				v->Push(LVObjectPtr(tofloat(res)));
				break;
			}
		}
		return lv_throwerror(v, _LC("cannot convert the string"));
		break;
		case OT_INTEGER:
		case OT_FLOAT:
			v->Push(LVObjectPtr(tofloat(o)));
			break;
		case OT_BOOL:
			v->Push(LVObjectPtr((LVFloat)(_integer(o) ? 1 : 0)));
			break;
		default:
			v->PushNull();
			break;
	}
	return 1;
}

static LVInteger default_delegate_tointeger(VMHANDLE v) {
	LVObjectPtr& o = stack_get(v, 1);
	LVInteger base = 10;
	if (lv_gettop(v) > 1) {
		lv_getinteger(v, 2, &base);
	}
	switch (type(o)) {
		case OT_STRING: {
			LVObjectPtr res;
			if (str2num(_stringval(o), res, base)) {
				v->Push(LVObjectPtr(tointeger(res)));
				break;
			}
		}
		return lv_throwerror(v, _LC("cannot convert the string"));
		break;
		case OT_INTEGER:
		case OT_FLOAT:
			v->Push(LVObjectPtr(tointeger(o)));
			break;
		case OT_BOOL:
			v->Push(LVObjectPtr(_integer(o) ? (LVInteger)1 : (LVInteger)0));
			break;
		default:
			v->PushNull();
			break;
	}
	return 1;
}

static LVInteger default_delegate_tostring(VMHANDLE v) {
	if (LV_FAILED(lv_tostring(v, 1)))
		return LV_ERROR;
	return 1;
}

static LVInteger obj_delegate_weakref(VMHANDLE v) {
	lv_weakref(v, 1);
	return 1;
}

static LVInteger obj_clear(VMHANDLE v) {
	return lv_clear(v, -1);
}

static LVInteger number_delegate_tochar(VMHANDLE v) {
	LVObject& o = stack_get(v, 1);
	LVChar c = (LVChar)tointeger(o);
	v->Push(LVString::Create(_ss(v), (const LVChar *)&c, 1));
	return 1;
}

/////////////////////////////////////////////////////////////////
//TABLE DEFAULT DELEGATE

static LVInteger table_rawdelete(VMHANDLE v) {
	if (LV_FAILED(lv_rawdeleteslot(v, 1, LVTrue)))
		return LV_ERROR;
	return 1;
}

static LVInteger container_rawexists(VMHANDLE v) {
	if (LV_SUCCEEDED(lv_rawget(v, -2))) {
		lv_pushbool(v, LVTrue);
		return 1;
	}
	lv_pushbool(v, LVFalse);
	return 1;
}

static LVInteger container_rawset(VMHANDLE v) {
	return lv_rawset(v, -3);
}

static LVInteger container_rawget(VMHANDLE v) {
	return LV_SUCCEEDED(lv_rawget(v, -2)) ? 1 : LV_ERROR;
}

static LVInteger table_setdelegate(VMHANDLE v) {
	if (LV_FAILED(lv_setdelegate(v, -2)))
		return LV_ERROR;
	lv_push(v, -1); // -1 because setdelegate pops 1
	return 1;
}

static LVInteger table_getdelegate(VMHANDLE v) {
	return LV_SUCCEEDED(lv_getdelegate(v, -1)) ? 1 : LV_ERROR;
}

const LVRegFunction LVSharedState::_table_default_delegate_funcz[] = {
	{_LC("length"), default_delegate_len, 1, _LC("t")},
	{_LC("size"), default_delegate_len, 1, _LC("t")},
	{_LC("rawget"), container_rawget, 2, _LC("t")},
	{_LC("rawset"), container_rawset, 3, _LC("t")},
	{_LC("rawdelete"), table_rawdelete, 2, _LC("t")},
	{_LC("rawin"), container_rawexists, 2, _LC("t")},
	{_LC("weakref"), obj_delegate_weakref, 1, NULL },
	{_LC("tostring"), default_delegate_tostring, 1, _LC(".")},
	{_LC("clear"), obj_clear, 1, _LC(".")},
	{_LC("setdelegate"), table_setdelegate, 2, _LC(".t|o")},
	{_LC("getdelegate"), table_getdelegate, 1, _LC(".")},
	{NULL, (LVFUNCTION)0, 0, NULL}
};

//ARRAY DEFAULT DELEGATE///////////////////////////////////////

static LVInteger array_append(VMHANDLE v) {
	return lv_arrayappend(v, -2);
}

static LVInteger array_extend(VMHANDLE v) {
	_array(stack_get(v, 1))->Extend(_array(stack_get(v, 2)));
	return 0;
}

static LVInteger array_reverse(VMHANDLE v) {
	return lv_arrayreverse(v, -1);
}

static LVInteger array_pop(VMHANDLE v) {
	return LV_SUCCEEDED(lv_arraypop(v, 1, LVTrue)) ? 1 : LV_ERROR;
}

static LVInteger array_top(VMHANDLE v) {
	LVObject& o = stack_get(v, 1);
	if (_array(o)->Size() > 0) {
		v->Push(_array(o)->Top());
		return 1;
	} else return lv_throwerror(v, _LC("top() on a empty array"));
}

static LVInteger array_insert(VMHANDLE v) {
	LVObject& o = stack_get(v, 1);
	LVObject& idx = stack_get(v, 2);
	LVObject& val = stack_get(v, 3);
	if (!_array(o)->Insert(tointeger(idx), val))
		return lv_throwerror(v, _LC("index out of range"));
	return 0;
}

static LVInteger array_remove(VMHANDLE v) {
	LVObject& o = stack_get(v, 1);
	LVObject& idx = stack_get(v, 2);
	if (!lv_isnumeric(idx)) return lv_throwerror(v, _LC("wrong type"));
	LVObjectPtr val;
	if (_array(o)->Get(tointeger(idx), val)) {
		_array(o)->Remove(tointeger(idx));
		v->Push(val);
		return 1;
	}
	return lv_throwerror(v, _LC("idx out of range"));
}

static LVInteger array_resize(VMHANDLE v) {
	LVObject& o = stack_get(v, 1);
	LVObject& nsize = stack_get(v, 2);
	LVObjectPtr fill;
	if (lv_isnumeric(nsize)) {
		if (lv_gettop(v) > 2)
			fill = stack_get(v, 3);
		_array(o)->Resize(tointeger(nsize), fill);
		return 0;
	}
	return lv_throwerror(v, _LC("size must be a number"));
}

static LVInteger __map_array(LVArray *dest, LVArray *src, VMHANDLE v) {
	LVObjectPtr temp;
	LVInteger size = src->Size();
	for (LVInteger n = 0; n < size; n++) {
		src->Get(n, temp);
		v->Push(src);
		v->Push(temp);
		if (LV_FAILED(lv_call(v, 2, LVTrue, LVFalse))) {
			return LV_ERROR;
		}
		dest->Set(n, v->GetUp(-1));
		v->Pop();
	}
	return 0;
}

static LVInteger array_map(VMHANDLE v) {
	LVObject& o = stack_get(v, 1);
	LVInteger size = _array(o)->Size();
	LVObjectPtr ret = LVArray::Create(_ss(v), size);
	if (LV_FAILED(__map_array(_array(ret), _array(o), v)))
		return LV_ERROR;
	v->Push(ret);
	return 1;
}

static LVInteger array_apply(VMHANDLE v) {
	LVObject& o = stack_get(v, 1);
	if (LV_FAILED(__map_array(_array(o), _array(o), v)))
		return LV_ERROR;
	return 0;
}

static LVInteger array_reduce(VMHANDLE v) {
	LVObject& o = stack_get(v, 1);
	LVArray *a = _array(o);
	LVInteger size = a->Size();
	if (size == 0) {
		return 0;
	}
	LVObjectPtr res;
	a->Get(0, res);
	if (size > 1) {
		LVObjectPtr other;
		for (LVInteger n = 1; n < size; n++) {
			a->Get(n, other);
			v->Push(o);
			v->Push(res);
			v->Push(other);
			if (LV_FAILED(lv_call(v, 3, LVTrue, LVFalse))) {
				return LV_ERROR;
			}
			res = v->GetUp(-1);
			v->Pop();
		}
	}
	v->Push(res);
	return 1;
}

static LVInteger array_filter(VMHANDLE v) {
	LVObject& o = stack_get(v, 1);
	LVArray *a = _array(o);
	LVObjectPtr ret = LVArray::Create(_ss(v), 0);
	LVInteger size = a->Size();
	LVObjectPtr val;
	for (LVInteger n = 0; n < size; n++) {
		a->Get(n, val);
		v->Push(o);
		v->Push(n);
		v->Push(val);
		if (LV_FAILED(lv_call(v, 3, LVTrue, LVFalse))) {
			return LV_ERROR;
		}
		if (!LVVM::IsFalse(v->GetUp(-1))) {
			_array(ret)->Append(val);
		}
		v->Pop();
	}
	v->Push(ret);
	return 1;
}

static LVInteger array_find(VMHANDLE v) {
	LVObject& o = stack_get(v, 1);
	LVObjectPtr& val = stack_get(v, 2);
	LVArray *a = _array(o);
	LVInteger size = a->Size();
	LVObjectPtr temp;
	for (LVInteger n = 0; n < size; n++) {
		bool res = false;
		a->Get(n, temp);
		if (LVVM::IsEqual(temp, val, res) && res) {
			v->Push(n);
			return 1;
		}
	}
	return 0;
}

static bool _sort_compare(VMHANDLE v, LVObjectPtr& a, LVObjectPtr& b, LVInteger func, LVInteger& ret) {
	if (func < 0) {
		if (!v->ObjCmp(a, b, ret)) return false;
	} else {
		LVInteger top = lv_gettop(v);
		lv_push(v, func);
		lv_pushroottable(v);
		v->Push(a);
		v->Push(b);
		if (LV_FAILED(lv_call(v, 3, LVTrue, LVFalse))) {
			if (!lv_isstring( v->_lasterror))
				v->Raise_Error(_LC("compare func failed"));
			return false;
		}
		if (LV_FAILED(lv_getinteger(v, -1, &ret))) {
			v->Raise_Error(_LC("numeric value expected as return value of the compare function"));
			return false;
		}
		lv_settop(v, top);
		return true;
	}
	return true;
}

static bool _hsort_sift_down(VMHANDLE v, LVArray *arr, LVInteger root, LVInteger bottom, LVInteger func) {
	LVInteger maxChild;
	LVInteger done = 0;
	LVInteger ret;
	LVInteger root2;
	while (((root2 = root * 2) <= bottom) && (!done)) {
		if (root2 == bottom) {
			maxChild = root2;
		} else {
			if (!_sort_compare(v, arr->_values[root2], arr->_values[root2 + 1], func, ret))
				return false;
			if (ret > 0) {
				maxChild = root2;
			} else {
				maxChild = root2 + 1;
			}
		}

		if (!_sort_compare(v, arr->_values[root], arr->_values[maxChild], func, ret))
			return false;
		if (ret < 0) {
			if (root == maxChild) {
				v->Raise_Error(_LC("inconsistent compare function"));
				return false; // We'd be swapping ourselve. The compare function is incorrect
			}

			_Swap(arr->_values[root], arr->_values[maxChild]);
			root = maxChild;
		} else {
			done = 1;
		}
	}
	return true;
}

static bool _hsort(VMHANDLE v, LVObjectPtr& arr, LVInteger LV_UNUSED_ARG(l), LVInteger LV_UNUSED_ARG(r), LVInteger func) {
	LVArray *a = _array(arr);
	LVInteger i;
	LVInteger array_size = a->Size();
	for (i = (array_size / 2); i >= 0; i--) {
		if (!_hsort_sift_down(v, a, i, array_size - 1, func)) return false;
	}

	for (i = array_size - 1; i >= 1; i--) {
		_Swap(a->_values[0], a->_values[i]);
		if (!_hsort_sift_down(v, a, 0, i - 1, func)) return false;
	}
	return true;
}

static LVInteger array_sort(VMHANDLE v) {
	LVInteger func = -1;
	LVObjectPtr& o = stack_get(v, 1);
	if (_array(o)->Size() > 1) {
		if (lv_gettop(v) == 2)
			func = 2;
		if (!_hsort(v, o, 0, _array(o)->Size() - 1, func))
			return LV_ERROR;

	}
	return 0;
}

static LVInteger array_slice(VMHANDLE v) {
	LVInteger sidx, eidx;
	LVObjectPtr o;
	if (get_slice_params(v, sidx, eidx, o) == -1)
		return -1;
	LVInteger alen = _array(o)->Size();
	if (sidx < 0)
		sidx = alen + sidx;
	if (eidx < 0)
		eidx = alen + eidx;
	if (eidx < sidx)
		return lv_throwerror(v, _LC("wrong indexes"));
	if (eidx > alen || sidx < 0)
		return lv_throwerror(v, _LC("slice out of range"));
	LVArray *arr = LVArray::Create(_ss(v), eidx - sidx);
	LVObjectPtr t;
	LVInteger count = 0;
	for (LVInteger i = sidx; i < eidx; i++) {
		_array(o)->Get(i, t);
		arr->Set(count++, t);
	}
	v->Push(arr);
	return 1;
}

const LVRegFunction LVSharedState::_array_default_delegate_funcz[] = {
	{_LC("length"), default_delegate_len, 1, _LC("a")},
	{_LC("size"), default_delegate_len, 1, _LC("a")},
	{_LC("append"), array_append, 2, _LC("a")},
	{_LC("extend"), array_extend, 2, _LC("aa")},
	{_LC("push"), array_append, 2, _LC("a")},
	{_LC("pop"), array_pop, 1, _LC("a")},
	{_LC("top"), array_top, 1, _LC("a")},
	{_LC("insert"), array_insert, 3, _LC("an")},
	{_LC("remove"), array_remove, 2, _LC("an")},
	{_LC("resize"), array_resize, -2, _LC("an")},
	{_LC("reverse"), array_reverse, 1, _LC("a")},
	{_LC("sort"), array_sort, -1, _LC("ac")},
	{_LC("slice"), array_slice, -1, _LC("ann")},
	{_LC("weakref"), obj_delegate_weakref, 1, NULL },
	{_LC("tostring"), default_delegate_tostring, 1, _LC(".")},
	{_LC("clear"), obj_clear, 1, _LC(".")},
	{_LC("map"), array_map, 2, _LC("ac")},
	{_LC("apply"), array_apply, 2, _LC("ac")},
	{_LC("reduce"), array_reduce, 2, _LC("ac")},
	{_LC("filter"), array_filter, 2, _LC("ac")},
	{_LC("find"), array_find, 2, _LC("a.")},
	{NULL, (LVFUNCTION)0, 0, NULL}
};

//STRING DEFAULT DELEGATE//////////////////////////
static LVInteger string_slice(VMHANDLE v) {
	LVInteger sidx, eidx;
	LVObjectPtr o;
	if (LV_FAILED(get_slice_params(v, sidx, eidx, o)))
		return -1;

	LVInteger slen = _string(o)->_len;
	if (sidx < 0)
		sidx = slen + sidx;
	if (eidx < 0)
		eidx = slen + eidx;
	if (eidx < sidx)
		return lv_throwerror(v, _LC("wrong indexes"));
	if (eidx > slen || sidx < 0)
		return lv_throwerror(v, _LC("slice out of range"));

	v->Push(LVString::Create(_ss(v), &_stringval(o)[sidx], eidx - sidx));
	return 1;
}

static LVInteger string_find(VMHANDLE v) {
	LVInteger top, start_idx = 0;
	const LVChar *str, *substr, *ret;
	if (((top = lv_gettop(v)) > 1) && LV_SUCCEEDED(lv_getstring(v, 1, &str)) && LV_SUCCEEDED(lv_getstring(v, 2, &substr))) {
		if (top > 2)
			lv_getinteger(v, 3, &start_idx);
		if ((lv_getsize(v, 1) > start_idx) && (start_idx >= 0)) {
			ret = scstrstr(&str[start_idx], substr);
			if (ret) {
				lv_pushinteger(v, (LVInteger)(ret - str));
				return 1;
			}
		}
		return 0;
	}
	return lv_throwerror(v, _LC("invalid param"));
}

#define STRING_TOFUNCZ(func) static LVInteger string_##func(VMHANDLE v) \
{\
	LVInteger sidx,eidx; \
	LVObjectPtr str; \
	if (LV_FAILED(get_slice_params(v,sidx,eidx,str))) \
		return -1; \
	LVInteger slen = _string(str)->_len; \
	if (sidx < 0) \
		sidx = slen + sidx; \
	if (eidx < 0) \
		eidx = slen + eidx; \
	if (eidx < sidx) \
		return lv_throwerror(v,_LC("wrong indexes")); \
	if (eidx > slen || sidx < 0) \
		return lv_throwerror(v,_LC("slice out of range")); \
	LVInteger len=_string(str)->_len; \
	const LVChar *sthis = _stringval(str); \
	LVChar *snew = (_ss(v)->GetScratchPad(lv_rsl(len))); \
	memcpy(snew, sthis, lv_rsl(len));\
	for (LVInteger i=sidx; i<eidx; i++) \
		snew[i] = func(sthis[i]); \
	v->Push(LVString::Create(_ss(v),snew,len)); \
	return 1; \
}

STRING_TOFUNCZ(tolower)
STRING_TOFUNCZ(toupper)

const LVRegFunction LVSharedState::_string_default_delegate_funcz[] = {
	{_LC("length"), default_delegate_len, 1, _LC("s")},
	{_LC("size"), default_delegate_len, 1, _LC("s")},
	{_LC("tointeger"), default_delegate_tointeger, -1, _LC("sn")},
	{_LC("tofloat"), default_delegate_tofloat, 1, _LC("s")},
	{_LC("tostring"), default_delegate_tostring, 1, _LC(".")},
	{_LC("slice"), string_slice, -1, _LC("s n  n")},
	{_LC("find"), string_find, -2, _LC("s s n")},
	{_LC("tolower"), string_tolower, -1, _LC("s n n")},
	{_LC("toupper"), string_toupper, -1, _LC("s n n")},
	{_LC("weakref"), obj_delegate_weakref, 1, NULL },
	{NULL, (LVFUNCTION)0, 0, NULL}
};

//INTEGER DEFAULT DELEGATE//////////////////////////
const LVRegFunction LVSharedState::_number_default_delegate_funcz[] = {
	{_LC("tointeger"), default_delegate_tointeger, 1, _LC("n|b")},
	{_LC("tofloat"), default_delegate_tofloat, 1, _LC("n|b")},
	{_LC("tostring"), default_delegate_tostring, 1, _LC(".")},
	{_LC("tochar"), number_delegate_tochar, 1, _LC("n|b")},
	{_LC("weakref"), obj_delegate_weakref, 1, NULL },
	{NULL, (LVFUNCTION)0, 0, NULL}
};

//CLOSURE DEFAULT DELEGATE//////////////////////////
static LVInteger closure_pcall(VMHANDLE v) {
	return LV_SUCCEEDED(lv_call(v, lv_gettop(v) - 1, LVTrue, LVFalse)) ? 1 : LV_ERROR;
}

static LVInteger closure_call(VMHANDLE v) {
	return LV_SUCCEEDED(lv_call(v, lv_gettop(v) - 1, LVTrue, LVTrue)) ? 1 : LV_ERROR;
}

static LVInteger _closure_acall(VMHANDLE v, LVBool raiseerror) {
	LVArray *aparams = _array(stack_get(v, 2));
	LVInteger nparams = aparams->Size();
	v->Push(stack_get(v, 1));
	for (LVInteger i = 0; i < nparams; i++)v->Push(aparams->_values[i]);
	return LV_SUCCEEDED(lv_call(v, nparams, LVTrue, raiseerror)) ? 1 : LV_ERROR;
}

static LVInteger closure_acall(VMHANDLE v) {
	return _closure_acall(v, LVTrue);
}

static LVInteger closure_pacall(VMHANDLE v) {
	return _closure_acall(v, LVFalse);
}

static LVInteger closure_bindenv(VMHANDLE v) {
	if (LV_FAILED(lv_bindenv(v, 1)))
		return LV_ERROR;
	return 1;
}

static LVInteger closure_getroot(VMHANDLE v) {
	if (LV_FAILED(lv_getclosureroot(v, -1)))
		return LV_ERROR;
	return 1;
}

static LVInteger closure_setroot(VMHANDLE v) {
	if (LV_FAILED(lv_setclosureroot(v, -2)))
		return LV_ERROR;
	return 1;
}

static LVInteger closure_getinfos(VMHANDLE v) {
	LVObject o = stack_get(v, 1);
	LVTable *res = LVTable::Create(_ss(v), 4);
	if (type(o) == OT_CLOSURE) {
		FunctionPrototype *f = _closure(o)->_function;
		LVInteger nparams = f->_nparameters + (f->_varparams ? 1 : 0);
		LVObjectPtr params = LVArray::Create(_ss(v), nparams);
		LVObjectPtr defparams = LVArray::Create(_ss(v), f->_ndefaultparams);
		for (LVInteger n = 0; n < f->_nparameters; n++) {
			_array(params)->Set((LVInteger)n, f->_parameters[n]);
		}
		for (LVInteger j = 0; j < f->_ndefaultparams; j++) {
			_array(defparams)->Set((LVInteger)j, _closure(o)->_defaultparams[j]);
		}
		if (f->_varparams) {
			_array(params)->Set(nparams - 1, LVString::Create(_ss(v), _LC("..."), -1));
		}
		res->NewSlot(LVString::Create(_ss(v), _LC("native"), -1), false);
		res->NewSlot(LVString::Create(_ss(v), _LC("name"), -1), f->_name);
		res->NewSlot(LVString::Create(_ss(v), _LC("src"), -1), f->_sourcename);
		res->NewSlot(LVString::Create(_ss(v), _LC("parameters"), -1), params);
		res->NewSlot(LVString::Create(_ss(v), _LC("varargs"), -1), f->_varparams);
		res->NewSlot(LVString::Create(_ss(v), _LC("defparams"), -1), defparams);
	} else { //OT_NATIVECLOSURE
		LVNativeClosure *nc = _nativeclosure(o);
		res->NewSlot(LVString::Create(_ss(v), _LC("native"), -1), true);
		res->NewSlot(LVString::Create(_ss(v), _LC("name"), -1), nc->_name);
		res->NewSlot(LVString::Create(_ss(v), _LC("paramscheck"), -1), nc->_nparamscheck);
		LVObjectPtr typecheck;
		if (nc->_typecheck.size() > 0) {
			typecheck =
			    LVArray::Create(_ss(v), nc->_typecheck.size());
			for (LVUnsignedInteger n = 0; n < nc->_typecheck.size(); n++) {
				_array(typecheck)->Set((LVInteger)n, nc->_typecheck[n]);
			}
		}
		res->NewSlot(LVString::Create(_ss(v), _LC("typecheck"), -1), typecheck);
	}
	v->Push(res);
	return 1;
}

const LVRegFunction LVSharedState::_closure_default_delegate_funcz[] = {
	{_LC("call"), closure_call, -1, _LC("c")},
	{_LC("pcall"), closure_pcall, -1, _LC("c")},
	{_LC("acall"), closure_acall, 2, _LC("ca")},
	{_LC("pacall"), closure_pacall, 2, _LC("ca")},
	{_LC("weakref"), obj_delegate_weakref, 1, NULL },
	{_LC("tostring"), default_delegate_tostring, 1, _LC(".")},
	{_LC("bindenv"), closure_bindenv, 2, _LC("c x|y|t")},
	{_LC("getinfos"), closure_getinfos, 1, _LC("c")},
	{_LC("getroot"), closure_getroot, 1, _LC("c")},
	{_LC("setroot"), closure_setroot, 2, _LC("ct")},
	{NULL, (LVFUNCTION)0, 0, NULL}
};

//GENERATOR DEFAULT DELEGATE
static LVInteger generator_getstatus(VMHANDLE v) {
	LVObject& o = stack_get(v, 1);
	switch (_generator(o)->_state) {
		case LVGenerator::eSuspended:
			v->Push(LVString::Create(_ss(v), _LC("suspended")));
			break;
		case LVGenerator::eRunning:
			v->Push(LVString::Create(_ss(v), _LC("running")));
			break;
		case LVGenerator::eDead:
			v->Push(LVString::Create(_ss(v), _LC("dead")));
			break;
	}
	return 1;
}

const LVRegFunction LVSharedState::_generator_default_delegate_funcz[] = {
	{_LC("getstatus"), generator_getstatus, 1, _LC("g")},
	{_LC("weakref"), obj_delegate_weakref, 1, NULL },
	{_LC("tostring"), default_delegate_tostring, 1, _LC(".")},
	{NULL, (LVFUNCTION)0, 0, NULL}
};

//THREAD DEFAULT DELEGATE
static LVInteger thread_call(VMHANDLE v) {
	LVObjectPtr o = stack_get(v, 1);
	if (type(o) == OT_THREAD) {
		LVInteger nparams = lv_gettop(v);
		_thread(o)->Push(_thread(o)->_roottable);
		for (LVInteger i = 2; i < (nparams + 1); i++)
			lv_move(_thread(o), v, i);
		if (LV_SUCCEEDED(lv_call(_thread(o), nparams, LVTrue, LVTrue))) {
			lv_move(v, _thread(o), -1);
			lv_pop(_thread(o), 1);
			return 1;
		}
		v->_lasterror = _thread(o)->_lasterror;
		return LV_ERROR;
	}
	return lv_throwerror(v, _LC("wrong parameter"));
}

static LVInteger thread_wakeup(VMHANDLE v) {
	LVObjectPtr o = stack_get(v, 1);
	if (type(o) == OT_THREAD) {
		LVVM *thread = _thread(o);
		LVInteger state = lv_getvmstate(thread);
		if (state != LV_VMSTATE_SUSPENDED) {
			switch (state) {
				case LV_VMSTATE_IDLE:
					return lv_throwerror(v, _LC("cannot wakeup a idle thread"));
					break;
				case LV_VMSTATE_RUNNING:
					return lv_throwerror(v, _LC("cannot wakeup a running thread"));
					break;
			}
		}

		LVInteger wakeupret = lv_gettop(v) > 1 ? LVTrue : LVFalse;
		if (wakeupret) {
			lv_move(thread, v, 2);
		}
		if (LV_SUCCEEDED(lv_wakeupvm(thread, wakeupret, LVTrue, LVTrue, LVFalse))) {
			lv_move(v, thread, -1);
			lv_pop(thread, 1); //pop retval
			if (lv_getvmstate(thread) == LV_VMSTATE_IDLE) {
				lv_settop(thread, 1); //pop roottable
			}
			return 1;
		}
		lv_settop(thread, 1);
		v->_lasterror = thread->_lasterror;
		return LV_ERROR;
	}
	return lv_throwerror(v, _LC("wrong parameter"));
}

static LVInteger thread_wakeupthrow(VMHANDLE v) {
	LVObjectPtr o = stack_get(v, 1);
	if (type(o) == OT_THREAD) {
		LVVM *thread = _thread(o);
		LVInteger state = lv_getvmstate(thread);
		if (state != LV_VMSTATE_SUSPENDED) {
			switch (state) {
				case LV_VMSTATE_IDLE:
					return lv_throwerror(v, _LC("cannot wakeup a idle thread"));
					break;
				case LV_VMSTATE_RUNNING:
					return lv_throwerror(v, _LC("cannot wakeup a running thread"));
					break;
			}
		}

		lv_move(thread, v, 2);
		lv_throwobject(thread);
		LVBool rethrow_error = LVTrue;
		if (lv_gettop(v) > 2) {
			lv_getbool(v, 3, &rethrow_error);
		}
		if (LV_SUCCEEDED(lv_wakeupvm(thread, LVFalse, LVTrue, LVTrue, LVTrue))) {
			lv_move(v, thread, -1);
			lv_pop(thread, 1); //pop retval
			if (lv_getvmstate(thread) == LV_VMSTATE_IDLE) {
				lv_settop(thread, 1); //pop roottable
			}
			return 1;
		}
		lv_settop(thread, 1);
		if (rethrow_error) {
			v->_lasterror = thread->_lasterror;
			return LV_ERROR;
		}
		return LV_OK;
	}
	return lv_throwerror(v, _LC("wrong parameter"));
}

static LVInteger thread_getstatus(VMHANDLE v) {
	LVObjectPtr& o = stack_get(v, 1);
	switch (lv_getvmstate(_thread(o))) {
		case LV_VMSTATE_IDLE:
			lv_pushstring(v, _LC("idle"), -1);
			break;
		case LV_VMSTATE_RUNNING:
			lv_pushstring(v, _LC("running"), -1);
			break;
		case LV_VMSTATE_SUSPENDED:
			lv_pushstring(v, _LC("suspended"), -1);
			break;
		default:
			return lv_throwerror(v, _LC("internal VM error"));
	}
	return 1;
}

static LVInteger thread_getstackinfos(VMHANDLE v) {
	LVObjectPtr o = stack_get(v, 1);
	if (type(o) == OT_THREAD) {
		LVVM *thread = _thread(o);
		LVInteger threadtop = lv_gettop(thread);
		LVInteger level;
		lv_getinteger(v, -1, &level);
		LVRESULT res = __getcallstackinfos(thread, level);
		if (LV_FAILED(res)) {
			lv_settop(thread, threadtop);
			if (type(thread->_lasterror) == OT_STRING) {
				lv_throwerror(v, _stringval(thread->_lasterror));
			} else {
				lv_throwerror(v, _LC("unknown error"));
			}
		}
		if (res > 0) {
			//some result
			lv_move(v, thread, -1);
			lv_settop(thread, threadtop);
			return 1;
		}
		//no result
		lv_settop(thread, threadtop);
		return 0;

	}
	return lv_throwerror(v, _LC("wrong parameter"));
}

const LVRegFunction LVSharedState::_thread_default_delegate_funcz[] = {
	{_LC("call"), thread_call, -1, _LC("v")},
	{_LC("wakeup"), thread_wakeup, -1, _LC("v")},
	{_LC("wakeupthrow"), thread_wakeupthrow, -2, _LC("v.b")},
	{_LC("getstatus"), thread_getstatus, 1, _LC("v")},
	{_LC("weakref"), obj_delegate_weakref, 1, NULL },
	{_LC("getstackinfos"), thread_getstackinfos, 2, _LC("vn")},
	{_LC("tostring"), default_delegate_tostring, 1, _LC(".")},
	{NULL, (LVFUNCTION)0, 0, NULL}
};

static LVInteger class_getattributes(VMHANDLE v) {
	return LV_SUCCEEDED(lv_getattributes(v, -2)) ? 1 : LV_ERROR;
}

static LVInteger class_setattributes(VMHANDLE v) {
	return LV_SUCCEEDED(lv_setattributes(v, -3)) ? 1 : LV_ERROR;
}

static LVInteger class_instance(VMHANDLE v) {
	return LV_SUCCEEDED(lv_createinstance(v, -1)) ? 1 : LV_ERROR;
}

static LVInteger class_getbase(VMHANDLE v) {
	return LV_SUCCEEDED(lv_getbase(v, -1)) ? 1 : LV_ERROR;
}

static LVInteger class_newmember(VMHANDLE v) {
	LVInteger top = lv_gettop(v);
	LVBool bstatic = LVFalse;
	if (top == 5) {
		lv_tobool(v, -1, &bstatic);
		lv_pop(v, 1);
	}

	if (top < 4) {
		lv_pushnull(v);
	}
	return LV_SUCCEEDED(lv_newmember(v, -4, bstatic)) ? 1 : LV_ERROR;
}

static LVInteger class_rawnewmember(VMHANDLE v) {
	LVInteger top = lv_gettop(v);
	LVBool bstatic = LVFalse;
	if (top == 5) {
		lv_tobool(v, -1, &bstatic);
		lv_pop(v, 1);
	}

	if (top < 4) {
		lv_pushnull(v);
	}
	return LV_SUCCEEDED(lv_rawnewmember(v, -4, bstatic)) ? 1 : LV_ERROR;
}

const LVRegFunction LVSharedState::_class_default_delegate_funcz[] = {
	{_LC("getattributes"), class_getattributes, 2, _LC("y.")},
	{_LC("setattributes"), class_setattributes, 3, _LC("y..")},
	{_LC("rawget"), container_rawget, 2, _LC("y")},
	{_LC("rawset"), container_rawset, 3, _LC("y")},
	{_LC("rawin"), container_rawexists, 2, _LC("y")},
	{_LC("weakref"), obj_delegate_weakref, 1, NULL },
	{_LC("tostring"), default_delegate_tostring, 1, _LC(".")},
	{_LC("instance"), class_instance, 1, _LC("y")},
	{_LC("getbase"), class_getbase, 1, _LC("y")},
	{_LC("newmember"), class_newmember, -3, _LC("y")},
	{_LC("rawnewmember"), class_rawnewmember, -3, _LC("y")},
	{NULL, (LVFUNCTION)0, 0, NULL}
};

static LVInteger instance_getclass(VMHANDLE v) {
	if (LV_SUCCEEDED(lv_getclass(v, 1)))
		return 1;
	return LV_ERROR;
}

const LVRegFunction LVSharedState::_instance_default_delegate_funcz[] = {
	{_LC("getclass"), instance_getclass, 1, _LC("x")},
	{_LC("rawget"), container_rawget, 2, _LC("x")},
	{_LC("rawset"), container_rawset, 3, _LC("x")},
	{_LC("rawin"), container_rawexists, 2, _LC("x")},
	{_LC("weakref"), obj_delegate_weakref, 1, NULL },
	{_LC("tostring"), default_delegate_tostring, 1, _LC(".")},
	{NULL, (LVFUNCTION)0, 0, NULL}
};

static LVInteger weakref_ref(VMHANDLE v) {
	if (LV_FAILED(lv_getweakrefval(v, 1)))
		return LV_ERROR;
	return 1;
}

const LVRegFunction LVSharedState::_weakref_default_delegate_funcz[] = {
	{_LC("ref"), weakref_ref, 1, _LC("r")},
	{_LC("weakref"), obj_delegate_weakref, 1, NULL },
	{_LC("tostring"), default_delegate_tostring, 1, _LC(".")},
	{NULL, (LVFUNCTION)0, 0, NULL}
};
