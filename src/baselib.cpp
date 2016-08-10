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

static bool str2num(const SQChar *s, SQObjectPtr& res, SQInteger base) {
	SQChar *end;
	const SQChar *e = s;
	bool iseintbase = base > 13; //to fix error converting hexadecimals with e like 56f0791e
	bool isfloat = false;
	SQChar c;
	while ((c = *e) != _LC('\0')) {
		if (c == _LC('.') || (!iseintbase && (c == _LC('E') || c == _LC('e')))) { //e and E is for scientific notation
			isfloat = true;
			break;
		}
		e++;
	}
	if (isfloat) {
		SQFloat r = SQFloat(scstrtod(s, &end));
		if (s == end) return false;
		res = r;
	} else {
		SQInteger r = SQInteger(scstrtol(s, &end, (int)base));
		if (s == end) return false;
		res = r;
	}
	return true;
}

#define MAX_FORMAT_LEN  20
#define MAX_WFORMAT_LEN 3
#define ADDITIONAL_FORMAT_SPACE (100*sizeof(SQChar))

static SQBool isfmtchr(SQChar ch) {
	switch (ch) {
		case '-':
		case '+':
		case ' ':
		case '#':
		case '0':
			return SQTrue;
	}
	return SQFalse;
}

static SQInteger validate_format(HSQUIRRELVM v, SQChar *fmt, const SQChar *src, SQInteger n, SQInteger& width) {
	SQChar *dummy;
	SQChar swidth[MAX_WFORMAT_LEN];
	SQInteger wc = 0;
	SQInteger start = n;
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
	memcpy(&fmt[1], &src[start], ((n - start) + 1)*sizeof(SQChar));
	fmt[(n - start) + 2] = '\0';
	return n;
}

SQRESULT strformat(HSQUIRRELVM v, SQInteger nformatstringidx, SQInteger *outlen, SQChar **output) {
	const SQChar *format;
	SQChar *dest;
	SQChar fmt[MAX_FORMAT_LEN];
	lv_getstring(v, nformatstringidx, &format);
	SQInteger format_size = lv_getsize(v, nformatstringidx);
	SQInteger allocated = (format_size + 2) * sizeof(SQChar);
	dest = lv_getscratchpad(v, allocated);
	SQInteger n = 0, i = 0, nparam = nformatstringidx + 1, w = 0;

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
			SQInteger addlen = 0;
			SQInteger valtype = 0;
			const SQChar *ts = NULL;
			SQInteger ti = 0;
			SQFloat tf = 0;
			switch (format[n]) {
				case 's':
					if (LV_FAILED(lv_getstring(v, nparam, &ts)))
						return lv_throwerror(v, _LC("string expected for the specified format"));
					addlen = (lv_getsize(v, nparam) * sizeof(SQChar)) + ((w + 1) * sizeof(SQChar));
					valtype = 's';
					break;
				case 'i':
				case 'd':
				case 'o':
				case 'u':
				case 'x':
				case 'X':
#ifdef _SQ64
				{
					size_t flen = scstrlen(fmt);
					SQInteger fpos = flen - 1;
					SQChar f = fmt[fpos];
					const SQChar *prec = (const SQChar *)_PRINT_INT_PREC;
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
					addlen = (ADDITIONAL_FORMAT_SPACE) + ((w + 1) * sizeof(SQChar));
					valtype = 'i';
					break;
				case 'f':
				case 'g':
				case 'G':
				case 'e':
				case 'E':
					if (LV_FAILED(lv_getfloat(v, nparam, &tf)))
						return lv_throwerror(v, _LC("float expected for the specified format"));
					addlen = (ADDITIONAL_FORMAT_SPACE) + ((w + 1) * sizeof(SQChar));
					valtype = 'f';
					break;
				default:
					return lv_throwerror(v, _LC("invalid format"));
			}
			n++;
			allocated += addlen + sizeof(SQChar);
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

static SQInteger base_blub(HSQUIRRELVM LV_UNUSED_ARG(v)) {
	return 0;
}

#ifndef NO_GARBAGE_COLLECTOR
static SQInteger base_collectgarbage(HSQUIRRELVM v) {
	lv_pushinteger(v, lv_collectgarbage(v));
	return 1;
}
static SQInteger base_resurectureachable(HSQUIRRELVM v) {
	lv_resurrectunreachable(v);
	return 1;
}
#endif

static SQInteger base_getroottable(HSQUIRRELVM v) {
	v->Push(v->_roottable);
	return 1;
}

static SQInteger base_getconsttable(HSQUIRRELVM v) {
	v->Push(_ss(v)->_consts);
	return 1;
}

static SQInteger base_setroottable(HSQUIRRELVM v) {
	SQObjectPtr o = v->_roottable;
	if (LV_FAILED(lv_setroottable(v)))
		return LV_ERROR;
	v->Push(o);
	return 1;
}

static SQInteger base_setconsttable(HSQUIRRELVM v) {
	SQObjectPtr o = _ss(v)->_consts;
	if (LV_FAILED(lv_setconsttable(v)))
		return LV_ERROR;
	v->Push(o);
	return 1;
}

static SQInteger base_seterrorhandler(HSQUIRRELVM v) {
	lv_seterrorhandler(v);
	return 0;
}

static SQInteger base_setdebughook(HSQUIRRELVM v) {
	lv_setdebughook(v);
	return 0;
}

static SQInteger base_enabledebuginfo(HSQUIRRELVM v) {
	SQObjectPtr& o = stack_get(v, 2);

	lv_enabledebuginfo(v, SQVM::IsFalse(o) ? SQFalse : SQTrue);
	return 0;
}

static SQInteger __getcallstackinfos(HSQUIRRELVM v, SQInteger level) {
	SQStackInfos si;
	SQInteger seq = 0;
	const SQChar *name = NULL;

	if (LV_SUCCEEDED(lv_stackinfos(v, level, &si))) {
		const SQChar *fn = _LC("unknown");
		const SQChar *src = _LC("unknown");
		if (si.funcname)
			fn = si.funcname;
		if (si.source)
			src = si.source;
		lv_newtable(v);
		lv_pushstring(v, _LC("func"), -1);
		lv_pushstring(v, fn, -1);
		lv_newslot(v, -3, SQFalse);
		lv_pushstring(v, _LC("src"), -1);
		lv_pushstring(v, src, -1);
		lv_newslot(v, -3, SQFalse);
		lv_pushstring(v, _LC("line"), -1);
		lv_pushinteger(v, si.line);
		lv_newslot(v, -3, SQFalse);
		lv_pushstring(v, _LC("variables"), -1);
		lv_newtable(v);
		seq = 0;
		while ((name = lv_getlocal(v, level, seq))) {
			lv_pushstring(v, name, -1);
			lv_push(v, -2);
			lv_newslot(v, -4, SQFalse);
			lv_pop(v, 1);
			seq++;
		}
		lv_newslot(v, -3, SQFalse);
		return 1;
	}

	return 0;
}
static SQInteger base_getstackinfos(HSQUIRRELVM v) {
	SQInteger level;
	lv_getinteger(v, -1, &level);
	return __getcallstackinfos(v, level);
}

static SQInteger base_assert(HSQUIRRELVM v) {
	if (SQVM::IsFalse(stack_get(v, 2))) {
		return lv_throwerror(v, _LC("assertion failed"));
	}
	return 0;
}

static SQInteger get_slice_params(HSQUIRRELVM v, SQInteger& sidx, SQInteger& eidx, SQObjectPtr& o) {
	SQInteger top = lv_gettop(v);
	sidx = 0;
	eidx = 0;
	o = stack_get(v, 1);
	if (top > 1) {
		SQObjectPtr& start = stack_get(v, 2);
		if (type(start) != OT_NULL && sq_isnumeric(start)) {
			sidx = tointeger(start);
		}
	}
	if (top > 2) {
		SQObjectPtr& end = stack_get(v, 3);
		if (sq_isnumeric(end)) {
			eidx = tointeger(end);
		}
	} else {
		eidx = lv_getsize(v, 1);
	}
	return 1;
}

static SQInteger base_print(HSQUIRRELVM v) {
	const SQChar *str;
	if (LV_SUCCEEDED(lv_tostring(v, 2))) {
		if (LV_SUCCEEDED(lv_getstring(v, -1, &str))) {
			if (_ss(v)->_printfunc)
				_ss(v)->_printfunc(v, _LC("%s"), str);
			return 0;
		}
	}
	return LV_ERROR;
}

static SQInteger base_printf(HSQUIRRELVM v) {
	SQChar *dest;
	SQInteger length = 0;
	if (LV_FAILED(strformat(v, 2, &length, &dest)))
		return LV_ERROR;
	if (_ss(v)->_printfunc)
		_ss(v)->_printfunc(v, _LC("%s"), dest);
	return 0;
}

static SQInteger base_println(HSQUIRRELVM v) {
	const SQChar *str;
	if (LV_SUCCEEDED(lv_tostring(v, 2))) {
		if (LV_SUCCEEDED(lv_getstring(v, -1, &str))) {
			if (_ss(v)->_printfunc)
				_ss(v)->_printfunc(v, _LC("%s\n"), str);
			return 0;
		}
	}
	return LV_ERROR;
}

static SQInteger base_error(HSQUIRRELVM v) {
	const SQChar *str;
	if (LV_SUCCEEDED(lv_tostring(v, 2))) {
		if (LV_SUCCEEDED(lv_getstring(v, -1, &str))) {
			if (_ss(v)->_errorfunc)
				_ss(v)->_errorfunc(v, _LC("%s"), str);
			return 0;
		}
	}
	return LV_ERROR;
}

static SQInteger base_compilestring(HSQUIRRELVM v) {
	SQInteger nargs = lv_gettop(v);
	const SQChar *src = NULL, *name = _LC("unnamedbuffer");
	SQInteger size;
	lv_getstring(v, 2, &src);
	size = lv_getsize(v, 2);
	if (nargs > 2) {
		lv_getstring(v, 3, &name);
	}
	if (LV_SUCCEEDED(lv_compilebuffer(v, src, size, name, SQFalse)))
		return 1;
	else
		return LV_ERROR;
}

static SQInteger base_newthread(HSQUIRRELVM v) {
	SQObjectPtr& func = stack_get(v, 2);
	SQInteger stksize = (_closure(func)->_function->_stacksize << 1) + 2;
	HSQUIRRELVM newv = lv_newthread(v, (stksize < MIN_STACK_OVERHEAD + 2) ? MIN_STACK_OVERHEAD + 2 : stksize);
	lv_move(newv, v, -2);
	return 1;
}

static SQInteger base_suspend(HSQUIRRELVM v) {
	return lv_suspendvm(v);
}

static SQInteger base_array(HSQUIRRELVM v) {
	SQArray *a;
	SQObject& size = stack_get(v, 2);
	if (lv_gettop(v) > 2) {
		a = SQArray::Create(_ss(v), 0);
		a->Resize(tointeger(size), stack_get(v, 3));
	} else {
		a = SQArray::Create(_ss(v), tointeger(size));
	}
	v->Push(a);
	return 1;
}

static SQInteger base_type(HSQUIRRELVM v) {
	SQObjectPtr& o = stack_get(v, 2);
	v->Push(SQString::Create(_ss(v), GetTypeName(o), -1));
	return 1;
}

static SQInteger base_callee(HSQUIRRELVM v) {
	if (v->_callsstacksize > 1) {
		v->Push(v->_callsstack[v->_callsstacksize - 2]._closure);
		return 1;
	}
	return lv_throwerror(v, _LC("no closure in the calls stack"));
}

static const SQRegFunction base_funcs[] = {
	{_LC("seterrorhandler"), base_seterrorhandler, 2, NULL},
	{_LC("setdebughook"), base_setdebughook, 2, NULL},
	{_LC("enabledebuginfo"), base_enabledebuginfo, 2, NULL},
	{_LC("getstackinfos"), base_getstackinfos, 2, _LC(".n")},
	{_LC("getroottable"), base_getroottable, 1, NULL},
	{_LC("setroottable"), base_setroottable, 2, NULL},
	{_LC("getconsttable"), base_getconsttable, 1, NULL},
	{_LC("setconsttable"), base_setconsttable, 2, NULL},
	{_LC("assert"), base_assert, 2, NULL},
	{_LC("echo"), base_print, 2, NULL},
	{_LC("print"), base_print, 2, NULL},
	{_LC("printf"), base_printf, -2, ".s"},
	{_LC("println"), base_println, 2, NULL},
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
	{NULL, (SQFUNCTION)0, 0, NULL}
};

void lv_base_register(HSQUIRRELVM v) {
	SQInteger i = 0;
	lv_pushroottable(v);

	while (base_funcs[i].name != 0) {
		lv_pushstring(v, base_funcs[i].name, -1);
		lv_newclosure(v, base_funcs[i].f, 0);
		lv_setnativeclosurename(v, -1, base_funcs[i].name);
		lv_setparamscheck(v, base_funcs[i].nparamscheck, base_funcs[i].typemask);
		lv_newslot(v, -3, SQFalse);
		i++;
	}

	lv_pushstring(v, _LC("_versionnumber_"), -1);
	lv_pushinteger(v, LAVRIL_VERSION_NUMBER);
	lv_newslot(v, -3, SQFalse);
	lv_pushstring(v, _LC("_version_"), -1);
	lv_pushstring(v, LAVRIL_VERSION, -1);
	lv_newslot(v, -3, SQFalse);
	lv_pushstring(v, _LC("_charsize_"), -1);
	lv_pushinteger(v, sizeof(SQChar));
	lv_newslot(v, -3, SQFalse);
	lv_pushstring(v, _LC("_intsize_"), -1);
	lv_pushinteger(v, sizeof(SQInteger));
	lv_newslot(v, -3, SQFalse);
	lv_pushstring(v, _LC("_floatsize_"), -1);
	lv_pushinteger(v, sizeof(SQFloat));
	lv_newslot(v, -3, SQFalse);
	lv_pop(v, 1);
}

static SQInteger default_delegate_len(HSQUIRRELVM v) {
	v->Push(SQInteger(lv_getsize(v, 1)));
	return 1;
}

static SQInteger default_delegate_tofloat(HSQUIRRELVM v) {
	SQObjectPtr& o = stack_get(v, 1);
	switch (type(o)) {
		case OT_STRING: {
			SQObjectPtr res;
			if (str2num(_stringval(o), res, 10)) {
				v->Push(SQObjectPtr(tofloat(res)));
				break;
			}
		}
		return lv_throwerror(v, _LC("cannot convert the string"));
		break;
		case OT_INTEGER:
		case OT_FLOAT:
			v->Push(SQObjectPtr(tofloat(o)));
			break;
		case OT_BOOL:
			v->Push(SQObjectPtr((SQFloat)(_integer(o) ? 1 : 0)));
			break;
		default:
			v->PushNull();
			break;
	}
	return 1;
}

static SQInteger default_delegate_tointeger(HSQUIRRELVM v) {
	SQObjectPtr& o = stack_get(v, 1);
	SQInteger base = 10;
	if (lv_gettop(v) > 1) {
		lv_getinteger(v, 2, &base);
	}
	switch (type(o)) {
		case OT_STRING: {
			SQObjectPtr res;
			if (str2num(_stringval(o), res, base)) {
				v->Push(SQObjectPtr(tointeger(res)));
				break;
			}
		}
		return lv_throwerror(v, _LC("cannot convert the string"));
		break;
		case OT_INTEGER:
		case OT_FLOAT:
			v->Push(SQObjectPtr(tointeger(o)));
			break;
		case OT_BOOL:
			v->Push(SQObjectPtr(_integer(o) ? (SQInteger)1 : (SQInteger)0));
			break;
		default:
			v->PushNull();
			break;
	}
	return 1;
}

static SQInteger default_delegate_tostring(HSQUIRRELVM v) {
	if (LV_FAILED(lv_tostring(v, 1)))
		return LV_ERROR;
	return 1;
}

static SQInteger obj_delegate_weakref(HSQUIRRELVM v) {
	lv_weakref(v, 1);
	return 1;
}

static SQInteger obj_clear(HSQUIRRELVM v) {
	return lv_clear(v, -1);
}

static SQInteger number_delegate_tochar(HSQUIRRELVM v) {
	SQObject& o = stack_get(v, 1);
	SQChar c = (SQChar)tointeger(o);
	v->Push(SQString::Create(_ss(v), (const SQChar *)&c, 1));
	return 1;
}

/////////////////////////////////////////////////////////////////
//TABLE DEFAULT DELEGATE

static SQInteger table_rawdelete(HSQUIRRELVM v) {
	if (LV_FAILED(lv_rawdeleteslot(v, 1, SQTrue)))
		return LV_ERROR;
	return 1;
}

static SQInteger container_rawexists(HSQUIRRELVM v) {
	if (LV_SUCCEEDED(lv_rawget(v, -2))) {
		lv_pushbool(v, SQTrue);
		return 1;
	}
	lv_pushbool(v, SQFalse);
	return 1;
}

static SQInteger container_rawset(HSQUIRRELVM v) {
	return lv_rawset(v, -3);
}

static SQInteger container_rawget(HSQUIRRELVM v) {
	return LV_SUCCEEDED(lv_rawget(v, -2)) ? 1 : LV_ERROR;
}

static SQInteger table_setdelegate(HSQUIRRELVM v) {
	if (LV_FAILED(lv_setdelegate(v, -2)))
		return LV_ERROR;
	lv_push(v, -1); // -1 because sq_setdelegate pops 1
	return 1;
}

static SQInteger table_getdelegate(HSQUIRRELVM v) {
	return LV_SUCCEEDED(lv_getdelegate(v, -1)) ? 1 : LV_ERROR;
}

const SQRegFunction SQSharedState::_table_default_delegate_funcz[] = {
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
	{NULL, (SQFUNCTION)0, 0, NULL}
};

//ARRAY DEFAULT DELEGATE///////////////////////////////////////

static SQInteger array_append(HSQUIRRELVM v) {
	return lv_arrayappend(v, -2);
}

static SQInteger array_extend(HSQUIRRELVM v) {
	_array(stack_get(v, 1))->Extend(_array(stack_get(v, 2)));
	return 0;
}

static SQInteger array_reverse(HSQUIRRELVM v) {
	return lv_arrayreverse(v, -1);
}

static SQInteger array_pop(HSQUIRRELVM v) {
	return LV_SUCCEEDED(lv_arraypop(v, 1, SQTrue)) ? 1 : LV_ERROR;
}

static SQInteger array_top(HSQUIRRELVM v) {
	SQObject& o = stack_get(v, 1);
	if (_array(o)->Size() > 0) {
		v->Push(_array(o)->Top());
		return 1;
	} else return lv_throwerror(v, _LC("top() on a empty array"));
}

static SQInteger array_insert(HSQUIRRELVM v) {
	SQObject& o = stack_get(v, 1);
	SQObject& idx = stack_get(v, 2);
	SQObject& val = stack_get(v, 3);
	if (!_array(o)->Insert(tointeger(idx), val))
		return lv_throwerror(v, _LC("index out of range"));
	return 0;
}

static SQInteger array_remove(HSQUIRRELVM v) {
	SQObject& o = stack_get(v, 1);
	SQObject& idx = stack_get(v, 2);
	if (!sq_isnumeric(idx)) return lv_throwerror(v, _LC("wrong type"));
	SQObjectPtr val;
	if (_array(o)->Get(tointeger(idx), val)) {
		_array(o)->Remove(tointeger(idx));
		v->Push(val);
		return 1;
	}
	return lv_throwerror(v, _LC("idx out of range"));
}

static SQInteger array_resize(HSQUIRRELVM v) {
	SQObject& o = stack_get(v, 1);
	SQObject& nsize = stack_get(v, 2);
	SQObjectPtr fill;
	if (sq_isnumeric(nsize)) {
		if (lv_gettop(v) > 2)
			fill = stack_get(v, 3);
		_array(o)->Resize(tointeger(nsize), fill);
		return 0;
	}
	return lv_throwerror(v, _LC("size must be a number"));
}

static SQInteger __map_array(SQArray *dest, SQArray *src, HSQUIRRELVM v) {
	SQObjectPtr temp;
	SQInteger size = src->Size();
	for (SQInteger n = 0; n < size; n++) {
		src->Get(n, temp);
		v->Push(src);
		v->Push(temp);
		if (LV_FAILED(lv_call(v, 2, SQTrue, SQFalse))) {
			return LV_ERROR;
		}
		dest->Set(n, v->GetUp(-1));
		v->Pop();
	}
	return 0;
}

static SQInteger array_map(HSQUIRRELVM v) {
	SQObject& o = stack_get(v, 1);
	SQInteger size = _array(o)->Size();
	SQObjectPtr ret = SQArray::Create(_ss(v), size);
	if (LV_FAILED(__map_array(_array(ret), _array(o), v)))
		return LV_ERROR;
	v->Push(ret);
	return 1;
}

static SQInteger array_apply(HSQUIRRELVM v) {
	SQObject& o = stack_get(v, 1);
	if (LV_FAILED(__map_array(_array(o), _array(o), v)))
		return LV_ERROR;
	return 0;
}

static SQInteger array_reduce(HSQUIRRELVM v) {
	SQObject& o = stack_get(v, 1);
	SQArray *a = _array(o);
	SQInteger size = a->Size();
	if (size == 0) {
		return 0;
	}
	SQObjectPtr res;
	a->Get(0, res);
	if (size > 1) {
		SQObjectPtr other;
		for (SQInteger n = 1; n < size; n++) {
			a->Get(n, other);
			v->Push(o);
			v->Push(res);
			v->Push(other);
			if (LV_FAILED(lv_call(v, 3, SQTrue, SQFalse))) {
				return LV_ERROR;
			}
			res = v->GetUp(-1);
			v->Pop();
		}
	}
	v->Push(res);
	return 1;
}

static SQInteger array_filter(HSQUIRRELVM v) {
	SQObject& o = stack_get(v, 1);
	SQArray *a = _array(o);
	SQObjectPtr ret = SQArray::Create(_ss(v), 0);
	SQInteger size = a->Size();
	SQObjectPtr val;
	for (SQInteger n = 0; n < size; n++) {
		a->Get(n, val);
		v->Push(o);
		v->Push(n);
		v->Push(val);
		if (LV_FAILED(lv_call(v, 3, SQTrue, SQFalse))) {
			return LV_ERROR;
		}
		if (!SQVM::IsFalse(v->GetUp(-1))) {
			_array(ret)->Append(val);
		}
		v->Pop();
	}
	v->Push(ret);
	return 1;
}

static SQInteger array_find(HSQUIRRELVM v) {
	SQObject& o = stack_get(v, 1);
	SQObjectPtr& val = stack_get(v, 2);
	SQArray *a = _array(o);
	SQInteger size = a->Size();
	SQObjectPtr temp;
	for (SQInteger n = 0; n < size; n++) {
		bool res = false;
		a->Get(n, temp);
		if (SQVM::IsEqual(temp, val, res) && res) {
			v->Push(n);
			return 1;
		}
	}
	return 0;
}

static bool _sort_compare(HSQUIRRELVM v, SQObjectPtr& a, SQObjectPtr& b, SQInteger func, SQInteger& ret) {
	if (func < 0) {
		if (!v->ObjCmp(a, b, ret)) return false;
	} else {
		SQInteger top = lv_gettop(v);
		lv_push(v, func);
		lv_pushroottable(v);
		v->Push(a);
		v->Push(b);
		if (LV_FAILED(lv_call(v, 3, SQTrue, SQFalse))) {
			if (!sq_isstring( v->_lasterror))
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

static bool _hsort_sift_down(HSQUIRRELVM v, SQArray *arr, SQInteger root, SQInteger bottom, SQInteger func) {
	SQInteger maxChild;
	SQInteger done = 0;
	SQInteger ret;
	SQInteger root2;
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

static bool _hsort(HSQUIRRELVM v, SQObjectPtr& arr, SQInteger LV_UNUSED_ARG(l), SQInteger LV_UNUSED_ARG(r), SQInteger func) {
	SQArray *a = _array(arr);
	SQInteger i;
	SQInteger array_size = a->Size();
	for (i = (array_size / 2); i >= 0; i--) {
		if (!_hsort_sift_down(v, a, i, array_size - 1, func)) return false;
	}

	for (i = array_size - 1; i >= 1; i--) {
		_Swap(a->_values[0], a->_values[i]);
		if (!_hsort_sift_down(v, a, 0, i - 1, func)) return false;
	}
	return true;
}

static SQInteger array_sort(HSQUIRRELVM v) {
	SQInteger func = -1;
	SQObjectPtr& o = stack_get(v, 1);
	if (_array(o)->Size() > 1) {
		if (lv_gettop(v) == 2)
			func = 2;
		if (!_hsort(v, o, 0, _array(o)->Size() - 1, func))
			return LV_ERROR;

	}
	return 0;
}

static SQInteger array_slice(HSQUIRRELVM v) {
	SQInteger sidx, eidx;
	SQObjectPtr o;
	if (get_slice_params(v, sidx, eidx, o) == -1)
		return -1;
	SQInteger alen = _array(o)->Size();
	if (sidx < 0)
		sidx = alen + sidx;
	if (eidx < 0)
		eidx = alen + eidx;
	if (eidx < sidx)
		return lv_throwerror(v, _LC("wrong indexes"));
	if (eidx > alen || sidx < 0)
		return lv_throwerror(v, _LC("slice out of range"));
	SQArray *arr = SQArray::Create(_ss(v), eidx - sidx);
	SQObjectPtr t;
	SQInteger count = 0;
	for (SQInteger i = sidx; i < eidx; i++) {
		_array(o)->Get(i, t);
		arr->Set(count++, t);
	}
	v->Push(arr);
	return 1;
}

const SQRegFunction SQSharedState::_array_default_delegate_funcz[] = {
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
	{NULL, (SQFUNCTION)0, 0, NULL}
};

//STRING DEFAULT DELEGATE//////////////////////////
static SQInteger string_slice(HSQUIRRELVM v) {
	SQInteger sidx, eidx;
	SQObjectPtr o;
	if (LV_FAILED(get_slice_params(v, sidx, eidx, o)))
		return -1;

	SQInteger slen = _string(o)->_len;
	if (sidx < 0)
		sidx = slen + sidx;
	if (eidx < 0)
		eidx = slen + eidx;
	if (eidx < sidx)
		return lv_throwerror(v, _LC("wrong indexes"));
	if (eidx > slen || sidx < 0)
		return lv_throwerror(v, _LC("slice out of range"));

	v->Push(SQString::Create(_ss(v), &_stringval(o)[sidx], eidx - sidx));
	return 1;
}

static SQInteger string_find(HSQUIRRELVM v) {
	SQInteger top, start_idx = 0;
	const SQChar *str, *substr, *ret;
	if (((top = lv_gettop(v)) > 1) && LV_SUCCEEDED(lv_getstring(v, 1, &str)) && LV_SUCCEEDED(lv_getstring(v, 2, &substr))) {
		if (top > 2)
			lv_getinteger(v, 3, &start_idx);
		if ((lv_getsize(v, 1) > start_idx) && (start_idx >= 0)) {
			ret = scstrstr(&str[start_idx], substr);
			if (ret) {
				lv_pushinteger(v, (SQInteger)(ret - str));
				return 1;
			}
		}
		return 0;
	}
	return lv_throwerror(v, _LC("invalid param"));
}

#define STRING_TOFUNCZ(func) static SQInteger string_##func(HSQUIRRELVM v) \
{\
	SQInteger sidx,eidx; \
	SQObjectPtr str; \
	if (LV_FAILED(get_slice_params(v,sidx,eidx,str))) \
		return -1; \
	SQInteger slen = _string(str)->_len; \
	if (sidx < 0) \
		sidx = slen + sidx; \
	if (eidx < 0) \
		eidx = slen + eidx; \
	if (eidx < sidx) \
		return lv_throwerror(v,_LC("wrong indexes")); \
	if (eidx > slen || sidx < 0) \
		return lv_throwerror(v,_LC("slice out of range")); \
	SQInteger len=_string(str)->_len; \
	const SQChar *sthis = _stringval(str); \
	SQChar *snew = (_ss(v)->GetScratchPad(sq_rsl(len))); \
	memcpy(snew, sthis, sq_rsl(len));\
	for (SQInteger i=sidx; i<eidx; i++) \
		snew[i] = func(sthis[i]); \
	v->Push(SQString::Create(_ss(v),snew,len)); \
	return 1; \
}

STRING_TOFUNCZ(tolower)
STRING_TOFUNCZ(toupper)

const SQRegFunction SQSharedState::_string_default_delegate_funcz[] = {
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
	{NULL, (SQFUNCTION)0, 0, NULL}
};

//INTEGER DEFAULT DELEGATE//////////////////////////
const SQRegFunction SQSharedState::_number_default_delegate_funcz[] = {
	{_LC("tointeger"), default_delegate_tointeger, 1, _LC("n|b")},
	{_LC("tofloat"), default_delegate_tofloat, 1, _LC("n|b")},
	{_LC("tostring"), default_delegate_tostring, 1, _LC(".")},
	{_LC("tochar"), number_delegate_tochar, 1, _LC("n|b")},
	{_LC("weakref"), obj_delegate_weakref, 1, NULL },
	{NULL, (SQFUNCTION)0, 0, NULL}
};

//CLOSURE DEFAULT DELEGATE//////////////////////////
static SQInteger closure_pcall(HSQUIRRELVM v) {
	return LV_SUCCEEDED(lv_call(v, lv_gettop(v) - 1, SQTrue, SQFalse)) ? 1 : LV_ERROR;
}

static SQInteger closure_call(HSQUIRRELVM v) {
	return LV_SUCCEEDED(lv_call(v, lv_gettop(v) - 1, SQTrue, SQTrue)) ? 1 : LV_ERROR;
}

static SQInteger _closure_acall(HSQUIRRELVM v, SQBool raiseerror) {
	SQArray *aparams = _array(stack_get(v, 2));
	SQInteger nparams = aparams->Size();
	v->Push(stack_get(v, 1));
	for (SQInteger i = 0; i < nparams; i++)v->Push(aparams->_values[i]);
	return LV_SUCCEEDED(lv_call(v, nparams, SQTrue, raiseerror)) ? 1 : LV_ERROR;
}

static SQInteger closure_acall(HSQUIRRELVM v) {
	return _closure_acall(v, SQTrue);
}

static SQInteger closure_pacall(HSQUIRRELVM v) {
	return _closure_acall(v, SQFalse);
}

static SQInteger closure_bindenv(HSQUIRRELVM v) {
	if (LV_FAILED(lv_bindenv(v, 1)))
		return LV_ERROR;
	return 1;
}

static SQInteger closure_getroot(HSQUIRRELVM v) {
	if (LV_FAILED(lv_getclosureroot(v, -1)))
		return LV_ERROR;
	return 1;
}

static SQInteger closure_setroot(HSQUIRRELVM v) {
	if (LV_FAILED(lv_setclosureroot(v, -2)))
		return LV_ERROR;
	return 1;
}

static SQInteger closure_getinfos(HSQUIRRELVM v) {
	SQObject o = stack_get(v, 1);
	SQTable *res = SQTable::Create(_ss(v), 4);
	if (type(o) == OT_CLOSURE) {
		FunctionPrototype *f = _closure(o)->_function;
		SQInteger nparams = f->_nparameters + (f->_varparams ? 1 : 0);
		SQObjectPtr params = SQArray::Create(_ss(v), nparams);
		SQObjectPtr defparams = SQArray::Create(_ss(v), f->_ndefaultparams);
		for (SQInteger n = 0; n < f->_nparameters; n++) {
			_array(params)->Set((SQInteger)n, f->_parameters[n]);
		}
		for (SQInteger j = 0; j < f->_ndefaultparams; j++) {
			_array(defparams)->Set((SQInteger)j, _closure(o)->_defaultparams[j]);
		}
		if (f->_varparams) {
			_array(params)->Set(nparams - 1, SQString::Create(_ss(v), _LC("..."), -1));
		}
		res->NewSlot(SQString::Create(_ss(v), _LC("native"), -1), false);
		res->NewSlot(SQString::Create(_ss(v), _LC("name"), -1), f->_name);
		res->NewSlot(SQString::Create(_ss(v), _LC("src"), -1), f->_sourcename);
		res->NewSlot(SQString::Create(_ss(v), _LC("parameters"), -1), params);
		res->NewSlot(SQString::Create(_ss(v), _LC("varargs"), -1), f->_varparams);
		res->NewSlot(SQString::Create(_ss(v), _LC("defparams"), -1), defparams);
	} else { //OT_NATIVECLOSURE
		SQNativeClosure *nc = _nativeclosure(o);
		res->NewSlot(SQString::Create(_ss(v), _LC("native"), -1), true);
		res->NewSlot(SQString::Create(_ss(v), _LC("name"), -1), nc->_name);
		res->NewSlot(SQString::Create(_ss(v), _LC("paramscheck"), -1), nc->_nparamscheck);
		SQObjectPtr typecheck;
		if (nc->_typecheck.size() > 0) {
			typecheck =
			    SQArray::Create(_ss(v), nc->_typecheck.size());
			for (SQUnsignedInteger n = 0; n < nc->_typecheck.size(); n++) {
				_array(typecheck)->Set((SQInteger)n, nc->_typecheck[n]);
			}
		}
		res->NewSlot(SQString::Create(_ss(v), _LC("typecheck"), -1), typecheck);
	}
	v->Push(res);
	return 1;
}

const SQRegFunction SQSharedState::_closure_default_delegate_funcz[] = {
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
	{NULL, (SQFUNCTION)0, 0, NULL}
};

//GENERATOR DEFAULT DELEGATE
static SQInteger generator_getstatus(HSQUIRRELVM v) {
	SQObject& o = stack_get(v, 1);
	switch (_generator(o)->_state) {
		case SQGenerator::eSuspended:
			v->Push(SQString::Create(_ss(v), _LC("suspended")));
			break;
		case SQGenerator::eRunning:
			v->Push(SQString::Create(_ss(v), _LC("running")));
			break;
		case SQGenerator::eDead:
			v->Push(SQString::Create(_ss(v), _LC("dead")));
			break;
	}
	return 1;
}

const SQRegFunction SQSharedState::_generator_default_delegate_funcz[] = {
	{_LC("getstatus"), generator_getstatus, 1, _LC("g")},
	{_LC("weakref"), obj_delegate_weakref, 1, NULL },
	{_LC("tostring"), default_delegate_tostring, 1, _LC(".")},
	{NULL, (SQFUNCTION)0, 0, NULL}
};

//THREAD DEFAULT DELEGATE
static SQInteger thread_call(HSQUIRRELVM v) {
	SQObjectPtr o = stack_get(v, 1);
	if (type(o) == OT_THREAD) {
		SQInteger nparams = lv_gettop(v);
		_thread(o)->Push(_thread(o)->_roottable);
		for (SQInteger i = 2; i < (nparams + 1); i++)
			lv_move(_thread(o), v, i);
		if (LV_SUCCEEDED(lv_call(_thread(o), nparams, SQTrue, SQTrue))) {
			lv_move(v, _thread(o), -1);
			lv_pop(_thread(o), 1);
			return 1;
		}
		v->_lasterror = _thread(o)->_lasterror;
		return LV_ERROR;
	}
	return lv_throwerror(v, _LC("wrong parameter"));
}

static SQInteger thread_wakeup(HSQUIRRELVM v) {
	SQObjectPtr o = stack_get(v, 1);
	if (type(o) == OT_THREAD) {
		SQVM *thread = _thread(o);
		SQInteger state = lv_getvmstate(thread);
		if (state != SQ_VMSTATE_SUSPENDED) {
			switch (state) {
				case SQ_VMSTATE_IDLE:
					return lv_throwerror(v, _LC("cannot wakeup a idle thread"));
					break;
				case SQ_VMSTATE_RUNNING:
					return lv_throwerror(v, _LC("cannot wakeup a running thread"));
					break;
			}
		}

		SQInteger wakeupret = lv_gettop(v) > 1 ? SQTrue : SQFalse;
		if (wakeupret) {
			lv_move(thread, v, 2);
		}
		if (LV_SUCCEEDED(lv_wakeupvm(thread, wakeupret, SQTrue, SQTrue, SQFalse))) {
			lv_move(v, thread, -1);
			lv_pop(thread, 1); //pop retval
			if (lv_getvmstate(thread) == SQ_VMSTATE_IDLE) {
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

static SQInteger thread_wakeupthrow(HSQUIRRELVM v) {
	SQObjectPtr o = stack_get(v, 1);
	if (type(o) == OT_THREAD) {
		SQVM *thread = _thread(o);
		SQInteger state = lv_getvmstate(thread);
		if (state != SQ_VMSTATE_SUSPENDED) {
			switch (state) {
				case SQ_VMSTATE_IDLE:
					return lv_throwerror(v, _LC("cannot wakeup a idle thread"));
					break;
				case SQ_VMSTATE_RUNNING:
					return lv_throwerror(v, _LC("cannot wakeup a running thread"));
					break;
			}
		}

		lv_move(thread, v, 2);
		lv_throwobject(thread);
		SQBool rethrow_error = SQTrue;
		if (lv_gettop(v) > 2) {
			lv_getbool(v, 3, &rethrow_error);
		}
		if (LV_SUCCEEDED(lv_wakeupvm(thread, SQFalse, SQTrue, SQTrue, SQTrue))) {
			lv_move(v, thread, -1);
			lv_pop(thread, 1); //pop retval
			if (lv_getvmstate(thread) == SQ_VMSTATE_IDLE) {
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

static SQInteger thread_getstatus(HSQUIRRELVM v) {
	SQObjectPtr& o = stack_get(v, 1);
	switch (lv_getvmstate(_thread(o))) {
		case SQ_VMSTATE_IDLE:
			lv_pushstring(v, _LC("idle"), -1);
			break;
		case SQ_VMSTATE_RUNNING:
			lv_pushstring(v, _LC("running"), -1);
			break;
		case SQ_VMSTATE_SUSPENDED:
			lv_pushstring(v, _LC("suspended"), -1);
			break;
		default:
			return lv_throwerror(v, _LC("internal VM error"));
	}
	return 1;
}

static SQInteger thread_getstackinfos(HSQUIRRELVM v) {
	SQObjectPtr o = stack_get(v, 1);
	if (type(o) == OT_THREAD) {
		SQVM *thread = _thread(o);
		SQInteger threadtop = lv_gettop(thread);
		SQInteger level;
		lv_getinteger(v, -1, &level);
		SQRESULT res = __getcallstackinfos(thread, level);
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

const SQRegFunction SQSharedState::_thread_default_delegate_funcz[] = {
	{_LC("call"), thread_call, -1, _LC("v")},
	{_LC("wakeup"), thread_wakeup, -1, _LC("v")},
	{_LC("wakeupthrow"), thread_wakeupthrow, -2, _LC("v.b")},
	{_LC("getstatus"), thread_getstatus, 1, _LC("v")},
	{_LC("weakref"), obj_delegate_weakref, 1, NULL },
	{_LC("getstackinfos"), thread_getstackinfos, 2, _LC("vn")},
	{_LC("tostring"), default_delegate_tostring, 1, _LC(".")},
	{NULL, (SQFUNCTION)0, 0, NULL}
};

static SQInteger class_getattributes(HSQUIRRELVM v) {
	return LV_SUCCEEDED(lv_getattributes(v, -2)) ? 1 : LV_ERROR;
}

static SQInteger class_setattributes(HSQUIRRELVM v) {
	return LV_SUCCEEDED(lv_setattributes(v, -3)) ? 1 : LV_ERROR;
}

static SQInteger class_instance(HSQUIRRELVM v) {
	return LV_SUCCEEDED(lv_createinstance(v, -1)) ? 1 : LV_ERROR;
}

static SQInteger class_getbase(HSQUIRRELVM v) {
	return LV_SUCCEEDED(lv_getbase(v, -1)) ? 1 : LV_ERROR;
}

static SQInteger class_newmember(HSQUIRRELVM v) {
	SQInteger top = lv_gettop(v);
	SQBool bstatic = SQFalse;
	if (top == 5) {
		lv_tobool(v, -1, &bstatic);
		lv_pop(v, 1);
	}

	if (top < 4) {
		lv_pushnull(v);
	}
	return LV_SUCCEEDED(lv_newmember(v, -4, bstatic)) ? 1 : LV_ERROR;
}

static SQInteger class_rawnewmember(HSQUIRRELVM v) {
	SQInteger top = lv_gettop(v);
	SQBool bstatic = SQFalse;
	if (top == 5) {
		lv_tobool(v, -1, &bstatic);
		lv_pop(v, 1);
	}

	if (top < 4) {
		lv_pushnull(v);
	}
	return LV_SUCCEEDED(lv_rawnewmember(v, -4, bstatic)) ? 1 : LV_ERROR;
}

const SQRegFunction SQSharedState::_class_default_delegate_funcz[] = {
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
	{NULL, (SQFUNCTION)0, 0, NULL}
};

static SQInteger instance_getclass(HSQUIRRELVM v) {
	if (LV_SUCCEEDED(lv_getclass(v, 1)))
		return 1;
	return LV_ERROR;
}

const SQRegFunction SQSharedState::_instance_default_delegate_funcz[] = {
	{_LC("getclass"), instance_getclass, 1, _LC("x")},
	{_LC("rawget"), container_rawget, 2, _LC("x")},
	{_LC("rawset"), container_rawset, 3, _LC("x")},
	{_LC("rawin"), container_rawexists, 2, _LC("x")},
	{_LC("weakref"), obj_delegate_weakref, 1, NULL },
	{_LC("tostring"), default_delegate_tostring, 1, _LC(".")},
	{NULL, (SQFUNCTION)0, 0, NULL}
};

static SQInteger weakref_ref(HSQUIRRELVM v) {
	if (LV_FAILED(lv_getweakrefval(v, 1)))
		return LV_ERROR;
	return 1;
}

const SQRegFunction SQSharedState::_weakref_default_delegate_funcz[] = {
	{_LC("ref"), weakref_ref, 1, _LC("r")},
	{_LC("weakref"), obj_delegate_weakref, 1, NULL },
	{_LC("tostring"), default_delegate_tostring, 1, _LC(".")},
	{NULL, (SQFUNCTION)0, 0, NULL}
};
