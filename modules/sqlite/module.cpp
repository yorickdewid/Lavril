#include <lavril.h>
#include <stdlib.h>
#include <setjmp.h>
#include "sqlite3.h"

struct LVSQLITEObj {
	sqlite3 *_db;
	int _rc;
	char *_err_msg;
	void *_jmpbuf;
	const LVChar **_error;
};

#define OBJECT_INSTANCE(v) \
	LVSQLITEObj *self = NULL; \
	lv_getinstanceup(v,1,(LVUserPointer *)&self, 0);

void mod_sqlite_free(LVSQLITEObj *exp) {
	if (exp) {
		sqlite3_close(exp->_db);
		if (exp->_jmpbuf)
			lv_free(exp->_jmpbuf, sizeof(jmp_buf));
		lv_free(exp, sizeof(LVSQLITEObj));
	}
}

static void mod_sqlite_error(LVSQLITEObj *exp, const LVChar *error) {
	if (exp->_error)
		*exp->_error = error;
	longjmp(*((jmp_buf *)exp->_jmpbuf), -1);
}

static LVInteger mod_sqlite_releasehook(LVUserPointer p, LVInteger LV_UNUSED_ARG(size)) {
	LVSQLITEObj *self = ((LVSQLITEObj *)p);
	mod_sqlite_free(self);
	return 1;
}

/*static int mod_sqlite_callback(void *data, int argc, char **argv, char **azColName) {
	int i;
	VMHANDLE v = (VMHANDLE)data;
	lv_newtable(v);
	for (i = 0; i < argc; i++) {
		lv_pushstring(v, azColName[i], -1);
		lv_pushstring(v, argv[i] ? argv[i] : _LC("NULL"), -1);
		lv_rawset(v, -3);
	}
	return 0;
}*/

LVSQLITEObj *mod_sqlite_init(const LVChar *filename, const LVChar **error) {
	LVSQLITEObj *volatile exp = (LVSQLITEObj *)lv_malloc(sizeof(LVSQLITEObj));
	exp->_db = NULL;
	exp->_err_msg = NULL;
	exp->_error = error;
	exp->_jmpbuf = lv_malloc(sizeof(jmp_buf));
	if (setjmp(*((jmp_buf *)exp->_jmpbuf)) == 0) {
		exp->_rc = sqlite3_open(filename, &exp->_db);
		if (exp->_rc)
			mod_sqlite_error(exp, sqlite3_errmsg(exp->_db));
	} else {
		mod_sqlite_free(exp);
		return NULL;
	}
	return exp;
}

static LVInteger _sqlite_exec(VMHANDLE v) {
	OBJECT_INSTANCE(v);
	const LVChar *query;
	lv_getstring(v, 2, &query);
	self->_rc = sqlite3_exec(self->_db, query, NULL, NULL, NULL);
	if (self->_rc != SQLITE_OK) {
		return lv_throwerror(v, sqlite3_errmsg(self->_db));
	}
	lv_pushbool(v, LVTrue);
	return 1;
}

static LVInteger _sqlite_constructor(VMHANDLE v) {
	const LVChar *error, *location;
	lv_getstring(v, 2, &location);
	LVSQLITEObj *db = mod_sqlite_init(location, &error);
	if (!db)
		return lv_throwerror(v, error);
	lv_setinstanceup(v, 1, db);
	lv_setreleasehook(v, 1, mod_sqlite_releasehook);
	return 0;
}

static LVInteger _sqlite__typeof(VMHANDLE v) {
	lv_pushstring(v, _LC("curl"), -1);
	return 1;
}

#define _DECL_SQLITE_FUNC(name,nparams,tycheck) {_LC(#name),_sqlite_##name,nparams,tycheck}
static const LVRegFunction sqlitelib_funcs[] = {
	_DECL_SQLITE_FUNC(constructor, 2, _LC(".s")),
	_DECL_SQLITE_FUNC(exec, 2, _LC("xs")),
	_DECL_SQLITE_FUNC(_typeof, 1, _LC("x")),
	{NULL, (LVFUNCTION)0, 0, NULL}
};
#undef _DECL_SQLITE_FUNC

LVRESULT mod_init_sqlite(VMHANDLE v) {
	LVInteger i = 0;

	lv_pushstring(v, _LC("sqlite"), -1);
	lv_newclass(v, LVFalse);
	while (sqlitelib_funcs[i].name != 0) {
		lv_pushstring(v, sqlitelib_funcs[i].name, -1);
		lv_newclosure(v, sqlitelib_funcs[i].f, 0);
		lv_setparamscheck(v, sqlitelib_funcs[i].nparamscheck, sqlitelib_funcs[i].typemask);
		lv_setnativeclosurename(v, -1, sqlitelib_funcs[i].name);
		lv_newslot(v, -3, LVFalse);
		i++;
	}
	lv_newslot(v, -3, LVFalse);

	return LV_OK;
}
