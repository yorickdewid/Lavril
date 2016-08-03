#include <lavril.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef SQUNICODE
#include <wchar.h>
#define scgetenv _wgetenv
#define scsystem _wsystem
#define scasctime _wasctime
#define scremove _wremove
#define screname _wrename
#else
#define scgetenv getenv
#define scsystem system
#define scasctime asctime
#define scremove remove
#define screname rename
#endif

static SQInteger _system_getenv(HSQUIRRELVM v) {
	const SQChar *s;
	if (LV_SUCCEEDED(lv_getstring(v, 2, &s))) {
		lv_pushstring(v, scgetenv(s), -1);
		return 1;
	}
	return 0;
}

static SQInteger _system_system(HSQUIRRELVM v) {
	const SQChar *s;
	if (LV_SUCCEEDED(lv_getstring(v, 2, &s))) {
		lv_pushinteger(v, scsystem(s));
		return 1;
	}
	return lv_throwerror(v, _SC("wrong param"));
}

static SQInteger _system_clock(HSQUIRRELVM v) {
	lv_pushfloat(v, ((SQFloat)clock()) / (SQFloat)CLOCKS_PER_SEC);
	return 1;
}

static SQInteger _system_time(HSQUIRRELVM v) {
	SQInteger t = (SQInteger)time(NULL);
	lv_pushinteger(v, t);
	return 1;
}

static SQInteger _system_remove(HSQUIRRELVM v) {
	const SQChar *s;
	lv_getstring(v, 2, &s);
	if (scremove(s) == -1)
		return lv_throwerror(v, _SC("remove() failed"));
	return 0;
}

static SQInteger _system_rename(HSQUIRRELVM v) {
	const SQChar *oldn, *newn;
	lv_getstring(v, 2, &oldn);
	lv_getstring(v, 3, &newn);
	if (screname(oldn, newn) == -1)
		return lv_throwerror(v, _SC("rename() failed"));
	return 0;
}

static void _set_integer_slot(HSQUIRRELVM v, const SQChar *name, SQInteger val) {
	lv_pushstring(v, name, -1);
	lv_pushinteger(v, val);
	lv_rawset(v, -3);
}

static SQInteger _system_date(HSQUIRRELVM v) {
	time_t t;
	SQInteger it;
	SQInteger format = 'l';
	if (lv_gettop(v) > 1) {
		lv_getinteger(v, 2, &it);
		t = it;
		if (lv_gettop(v) > 2) {
			lv_getinteger(v, 3, (SQInteger *)&format);
		}
	} else {
		time(&t);
	}
	tm *date;
	if (format == 'u')
		date = gmtime(&t);
	else
		date = localtime(&t);
	if (!date)
		return lv_throwerror(v, _SC("crt api failure"));
	lv_newtable(v);
	_set_integer_slot(v, _SC("sec"), date->tm_sec);
	_set_integer_slot(v, _SC("min"), date->tm_min);
	_set_integer_slot(v, _SC("hour"), date->tm_hour);
	_set_integer_slot(v, _SC("day"), date->tm_mday);
	_set_integer_slot(v, _SC("month"), date->tm_mon);
	_set_integer_slot(v, _SC("year"), date->tm_year + 1900);
	_set_integer_slot(v, _SC("wday"), date->tm_wday);
	_set_integer_slot(v, _SC("yday"), date->tm_yday);
	return 1;
}

#define _DECL_FUNC(name,nparams,pmask) {_SC(#name),_system_##name,nparams,pmask}
static const SQRegFunction systemlib_funcs[] = {
	_DECL_FUNC(getenv, 2, _SC(".s")),
	_DECL_FUNC(system, 2, _SC(".s")),
	_DECL_FUNC(clock, 0, NULL),
	_DECL_FUNC(time, 1, NULL),
	_DECL_FUNC(date, -1, _SC(".nn")),
	_DECL_FUNC(remove, 2, _SC(".s")),
	_DECL_FUNC(rename, 3, _SC(".ss")),
	{NULL, (SQFUNCTION)0, 0, NULL}
};
#undef _DECL_FUNC

SQInteger mod_init_system(HSQUIRRELVM v) {
	SQInteger i = 0;
	while (systemlib_funcs[i].name != 0) {
		lv_pushstring(v, systemlib_funcs[i].name, -1);
		lv_newclosure(v, systemlib_funcs[i].f, 0);
		lv_setparamscheck(v, systemlib_funcs[i].nparamscheck, systemlib_funcs[i].typemask);
		lv_setnativeclosurename(v, -1, systemlib_funcs[i].name);
		lv_newslot(v, -3, SQFalse);
		i++;
	}
	return 1;
}
