#include "pcheader.h"
#include "lvstring.h"
#include <ctype.h>

static void __strip_l(const LVChar *str, const LVChar **start) {
	const LVChar *t = str;
	while (((*t) != '\0') && scisspace(*t)) {
		t++;
	}
	*start = t;
}

static void __strip_r(const LVChar *str, LVInteger len, const LVChar **end) {
	if (len == 0) {
		*end = str;
		return;
	}
	const LVChar *t = &str[len - 1];
	while (t >= str && scisspace(*t)) {
		t--;
	}
	*end = t + 1;
}

static LVInteger _string_strip(VMHANDLE v) {
	const LVChar *str, *start, *end;
	lv_getstring(v, 2, &str);
	LVInteger len = lv_getsize(v, 2);
	__strip_l(str, &start);
	__strip_r(str, len, &end);
	lv_pushstring(v, start, end - start);
	return 1;
}

static LVInteger _string_lstrip(VMHANDLE v) {
	const LVChar *str, *start;
	lv_getstring(v, 2, &str);
	__strip_l(str, &start);
	lv_pushstring(v, start, -1);
	return 1;
}

static LVInteger _string_rstrip(VMHANDLE v) {
	const LVChar *str, *end;
	lv_getstring(v, 2, &str);
	LVInteger len = lv_getsize(v, 2);
	__strip_r(str, len, &end);
	lv_pushstring(v, str, end - str);
	return 1;
}

static LVInteger _string_split(VMHANDLE v) {
	const LVChar *str, *seps;
	LVChar *stemp;
	lv_getstring(v, 2, &str);
	lv_getstring(v, 3, &seps);
	LVInteger sepsize = lv_getsize(v, 3);
	if (sepsize == 0)
		return lv_throwerror(v, _LC("empty separators string"));
	LVInteger memsize = (lv_getsize(v, 2) + 1) * sizeof(LVChar);
	stemp = lv_getscratchpad(v, memsize);
	memcpy(stemp, str, memsize);
	LVChar *start = stemp;
	LVChar *end = stemp;
	lv_newarray(v, 0);
	while (*end != '\0') {
		LVChar cur = *end;
		for (LVInteger i = 0; i < sepsize; i++) {
			if (cur == seps[i]) {
				*end = 0;
				lv_pushstring(v, start, -1);
				lv_arrayappend(v, -2);
				start = end + 1;
				break;
			}
		}
		end++;
	}
	if (end != start) {
		lv_pushstring(v, start, -1);
		lv_arrayappend(v, -2);
	}
	return 1;
}

static LVInteger _string_escape(VMHANDLE v) {
	const LVChar *str;
	LVChar *dest, *resstr;
	LVInteger size;
	lv_getstring(v, 2, &str);
	size = lv_getsize(v, 2);
	if (size == 0) {
		lv_push(v, 2);
		return 1;
	}
#ifdef LVUNICODE
#if WCHAR_SIZE == 2
	const LVChar *escpat = _LC("\\x%04x");
	const LVInteger maxescsize = 6;
#else //WCHAR_SIZE == 4
	const LVChar *escpat = _LC("\\x%08x");
	const LVInteger maxescsize = 10;
#endif
#else
	const LVChar *escpat = _LC("\\x%02x");
	const LVInteger maxescsize = 4;
#endif
	LVInteger destcharsize = (size * maxescsize); //assumes every char could be escaped
	resstr = dest = (LVChar *)lv_getscratchpad(v, destcharsize * sizeof(LVChar));
	LVChar c;
	LVChar escch;
	LVInteger escaped = 0;
	for (int n = 0; n < size; n++) {
		c = *str++;
		escch = 0;
		if (scisprint(c) || c == 0) {
			switch (c) {
				case '\a':
					escch = 'a';
					break;
				case '\b':
					escch = 'b';
					break;
				case '\t':
					escch = 't';
					break;
				case '\n':
					escch = 'n';
					break;
				case '\v':
					escch = 'v';
					break;
				case '\f':
					escch = 'f';
					break;
				case '\r':
					escch = 'r';
					break;
				case '\\':
					escch = '\\';
					break;
				case '\"':
					escch = '\"';
					break;
				case '\'':
					escch = '\'';
					break;
				case 0:
					escch = '0';
					break;
			}
			if (escch) {
				*dest++ = '\\';
				*dest++ = escch;
				escaped++;
			} else {
				*dest++ = c;
			}
		} else {

			dest += scsprintf(dest, destcharsize, escpat, c);
			escaped++;
		}
	}

	if (escaped) {
		lv_pushstring(v, resstr, dest - resstr);
	} else {
		lv_push(v, 2); //nothing escaped
	}
	return 1;
}

static LVInteger _string_startswith(VMHANDLE v) {
	const LVChar *str, *cmp;
	lv_getstring(v, 2, &str);
	lv_getstring(v, 3, &cmp);
	LVInteger len = lv_getsize(v, 2);
	LVInteger cmplen = lv_getsize(v, 3);
	LVBool ret = LVFalse;
	if (cmplen <= len) {
		ret = memcmp(str, cmp, lv_rsl(cmplen)) == 0 ? LVTrue : LVFalse;
	}
	lv_pushbool(v, ret);
	return 1;
}

static LVInteger _string_endswith(VMHANDLE v) {
	const LVChar *str, *cmp;
	lv_getstring(v, 2, &str);
	lv_getstring(v, 3, &cmp);
	LVInteger len = lv_getsize(v, 2);
	LVInteger cmplen = lv_getsize(v, 3);
	LVBool ret = LVFalse;
	if (cmplen <= len) {
		ret = memcmp(&str[len - cmplen], cmp, lv_rsl(cmplen)) == 0 ? LVTrue : LVFalse;
	}
	lv_pushbool(v, ret);
	return 1;
}

/*#ifdef REGEX
#define SETUP_REX(v) \
	SQRex *self = NULL; \
	sq_getinstanceup(v,1,(LVUserPointer *)&self,0);

static LVInteger _rexobj_releasehook(LVUserPointer p, LVInteger SQ_UNUSED_ARG(size)) {
	SQRex *self = ((SQRex *)p);
	sqstd_rex_free(self);
	return 1;
}

static LVInteger _regexp_match(VMHANDLE v) {
	SETUP_REX(v);
	const LVChar *str;
	lv_getstring(v, 2, &str);
	if (sqstd_rex_match(self, str) == LVTrue) {
		lv_pushbool(v, LVTrue);
		return 1;
	}
	lv_pushbool(v, LVFalse);
	return 1;
}

static void _addrexmatch(VMHANDLE v, const LVChar *str, const LVChar *begin, const LVChar *end) {
	sq_newtable(v);
	lv_pushstring(v, _LC("begin"), -1);
	lv_pushinteger(v, begin - str);
	sq_rawset(v, -3);
	lv_pushstring(v, _LC("end"), -1);
	lv_pushinteger(v, end - str);
	sq_rawset(v, -3);
}

static LVInteger _regexp_search(VMHANDLE v) {
	SETUP_REX(v);
	const LVChar *str, *begin, *end;
	LVInteger start = 0;
	lv_getstring(v, 2, &str);
	if (lv_gettop(v) > 2)
		lv_getinteger(v, 3, &start);
	if (sqstd_rex_search(self, str + start, &begin, &end) == LVTrue) {
		_addrexmatch(v, str, begin, end);
		return 1;
	}
	return 0;
}

static LVInteger _regexp_capture(VMHANDLE v) {
	SETUP_REX(v);
	const LVChar *str, *begin, *end;
	LVInteger start = 0;
	lv_getstring(v, 2, &str);
	if (lv_gettop(v) > 2)
		lv_getinteger(v, 3, &start);
	if (sqstd_rex_search(self, str + start, &begin, &end) == LVTrue) {
		LVInteger n = sqstd_rex_getsubexpcount(self);
		SQRexMatch match;
		sq_newarray(v, 0);
		for (LVInteger i = 0; i < n; i++) {
			sqstd_rex_getsubexp(self, i, &match);
			if (match.len > 0)
				_addrexmatch(v, str, match.begin, match.begin + match.len);
			else
				_addrexmatch(v, str, str, str); //empty match
			sq_arrayappend(v, -2);
		}
		return 1;
	}
	return 0;
}

static LVInteger _regexp_subexpcount(VMHANDLE v) {
	SETUP_REX(v);
	lv_pushinteger(v, sqstd_rex_getsubexpcount(self));
	return 1;
}

static LVInteger _regexp_constructor(VMHANDLE v) {
	const LVChar *error, *pattern;
	lv_getstring(v, 2, &pattern);
	SQRex *rex = sqstd_rex_compile(pattern, &error);
	if (!rex)
		return lv_throwerror(v, error);
	sq_setinstanceup(v, 1, rex);
	sq_setreleasehook(v, 1, _rexobj_releasehook);
	return 0;
}

static LVInteger _regexp__typeof(VMHANDLE v) {
	lv_pushstring(v, _LC("regexp"), -1);
	return 1;
}

#define _DECL_REX_FUNC(name,nparams,pmask) {_LC(#name),_regexp_##name,nparams,pmask}
static const SQRegFunction rexobj_funcs[] = {
	_DECL_REX_FUNC(constructor, 2, _LC(".s")),
	_DECL_REX_FUNC(search, -2, _LC("xsn")),
	_DECL_REX_FUNC(match, 2, _LC("xs")),
	_DECL_REX_FUNC(capture, -2, _LC("xsn")),
	_DECL_REX_FUNC(subexpcount, 1, _LC("x")),
	_DECL_REX_FUNC(_typeof, 1, _LC("x")),
	{NULL, (SQFUNCTION)0, 0, NULL}
};
#undef _DECL_REX_FUNC
#endif*/

#define _DECL_FUNC(name,nparams,pmask) {_LC(#name),_string_##name,nparams,pmask}
static const SQRegFunction stringlib_funcs[] = {
	_DECL_FUNC(strip, 2, _LC(".s")),
	_DECL_FUNC(lstrip, 2, _LC(".s")),
	_DECL_FUNC(rstrip, 2, _LC(".s")),
	_DECL_FUNC(split, 3, _LC(".ss")),
	_DECL_FUNC(escape, 2, _LC(".s")),
	_DECL_FUNC(startswith, 3, _LC(".ss")),
	_DECL_FUNC(endswith, 3, _LC(".ss")),
	{NULL, (SQFUNCTION)0, 0, NULL}
};
#undef _DECL_FUNC

LVInteger mod_init_string(VMHANDLE v) {
	LVInteger i = 0;

	/*#ifdef REGEX
		lv_pushstring(v, _LC("regexp"), -1);
		sq_newclass(v, LVFalse);
		while (rexobj_funcs[i].name != 0) {
			const SQRegFunction& f = rexobj_funcs[i];
			lv_pushstring(v, f.name, -1);
			lv_newclosure(v, f.f, 0);
			lv_setparamscheck(v, f.nparamscheck, f.typemask);
			lv_setnativeclosurename(v, -1, f.name);
			lv_newslot(v, -3, LVFalse);
			i++;
		}
		lv_newslot(v, -3, LVFalse);
	#endif*/

	i = 0;
	while (stringlib_funcs[i].name != 0) {
		lv_pushstring(v, stringlib_funcs[i].name, -1);
		lv_newclosure(v, stringlib_funcs[i].f, 0);
		lv_setparamscheck(v, stringlib_funcs[i].nparamscheck, stringlib_funcs[i].typemask);
		lv_setnativeclosurename(v, -1, stringlib_funcs[i].name);
		lv_newslot(v, -3, LVFalse);
		i++;
	}
	return 1;
}
