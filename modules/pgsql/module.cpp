#include <lavril.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <postgresql/libpq-fe.h>

struct LVPGSQLObj {
	const char *_conninfo;
	PGconn *_conn;
	PGresult *_res;
	void *_jmpbuf;
	int nFields;
	int i, j;
	const SQChar **_error;
};

#define OBJECT_INSTANCE(v) \
	LVPGSQLObj *self = NULL; \
	lv_getinstanceup(v,1,(LVUserPointer *)&self, 0);

void mod_pgsql_free(LVPGSQLObj *exp) {
	if (exp) {
		if (exp->_res)
			PQclear(exp->_res);
		PQfinish(exp->_conn);
		if (exp->_jmpbuf)
			lv_free(exp->_jmpbuf, sizeof(jmp_buf));
		lv_free(exp, sizeof(LVPGSQLObj));
	}
}

static void mod_pgsql_error(LVPGSQLObj *exp, const SQChar *error) {
	if (exp->_error)
		*exp->_error = error;
	longjmp(*((jmp_buf *)exp->_jmpbuf), -1);
}

static SQInteger mod_pgsql_releasehook(LVUserPointer p, SQInteger LV_UNUSED_ARG(size)) {
	LVPGSQLObj *self = ((LVPGSQLObj *)p);
	mod_pgsql_free(self);
	return 1;
}

LVPGSQLObj *mod_pgsql_init(VMHANDLE v, const SQChar *connstr, const SQChar **error) {
	LVPGSQLObj *volatile exp = (LVPGSQLObj *)lv_malloc(sizeof(LVPGSQLObj));
	exp->_conninfo = NULL;
	exp->_conn = NULL;
	exp->_res = NULL;
	exp->_error = error;
	exp->_jmpbuf = lv_malloc(sizeof(jmp_buf));
	if (setjmp(*((jmp_buf *)exp->_jmpbuf)) == 0) {
		exp->_conn = PQconnectdb(connstr);
		if (PQstatus(exp->_conn) != CONNECTION_OK) {
			char *pgerror = PQerrorMessage(exp->_conn);
			SQChar *temp = lv_getscratchpad(v, scstrlen(pgerror) + 1);
			scstrcpy(temp, pgerror);
			mod_pgsql_error(exp, temp);
		}
	} else {
		mod_pgsql_free(exp);
		return NULL;
	}
	return exp;
}

static SQInteger _pgsql_exec(VMHANDLE v) {
	OBJECT_INSTANCE(v);
	const SQChar *query;
	lv_getstring(v, 2, &query);
	self->_res = PQexec(self->_conn, query);
	if (PQresultStatus(self->_res) != PGRES_COMMAND_OK) {
		return lv_throwerror(v, PQerrorMessage(self->_conn));
	}
	PQclear(self->_res);
	self->_res = NULL;
	lv_pushbool(v, LVTrue);
	return 1;
}

static SQInteger _pgsql_query(VMHANDLE v) {
	OBJECT_INSTANCE(v);
	const SQChar *query;
	lv_getstring(v, 2, &query);
	self->_res = PQexec(self->_conn, query);
	if (PQresultStatus(self->_res) != PGRES_TUPLES_OK) {
		return lv_throwerror(v, PQerrorMessage(self->_conn));
	}
	lv_pushbool(v, LVTrue);
	return 1;
}

static SQInteger _pgsql_fetch(VMHANDLE v) {
	OBJECT_INSTANCE(v);
	lv_newarray(v, 0);
	for (SQInteger i = 0; i < PQntuples(self->_res); ++i) {
		lv_newtable(v);
		for (SQInteger j = 0; j < PQnfields(self->_res); ++j) {
			lv_pushstring(v, PQfname(self->_res, j), -1);
			lv_pushstring(v, PQgetvalue(self->_res, i, j), -1);
			lv_rawset(v, -3);
		}
		lv_arrayappend(v, -2);
	}
	return 1;
}

static SQInteger _pgsql_rows(VMHANDLE v) {
	OBJECT_INSTANCE(v);
	lv_pushinteger(v, PQntuples(self->_res));
	return 1;
}

static SQInteger _pgsql_version(VMHANDLE v) {
	OBJECT_INSTANCE(v);
	lv_pushinteger(v, PQserverVersion(self->_conn));
	return 1;
}

static SQInteger _pgsql_database(VMHANDLE v) {
	OBJECT_INSTANCE(v);
	lv_pushstring(v, PQdb(self->_conn), -1);
	return 1;
}

static SQInteger _pgsql_user(VMHANDLE v) {
	OBJECT_INSTANCE(v);
	lv_pushstring(v, PQuser(self->_conn), -1);
	return 1;
}

static SQInteger _pgsql_constructor(VMHANDLE v) {
	const SQChar *error, *connstr;
	lv_getstring(v, 2, &connstr);
	LVPGSQLObj *db = mod_pgsql_init(v, connstr, &error);
	if (!db) {
		return lv_throwerror(v, error);
	}
	lv_setinstanceup(v, 1, db);
	lv_setreleasehook(v, 1, mod_pgsql_releasehook);
	return 0;
}

static SQInteger _pgsql__typeof(VMHANDLE v) {
	lv_pushstring(v, _LC("pgsql"), -1);
	return 1;
}

#define _DECL_PGSQL_FUNC(name,nparams,tycheck) {_LC(#name),_pgsql_##name,nparams,tycheck}
static const SQRegFunction pgsqllib_funcs[] = {
	_DECL_PGSQL_FUNC(constructor, 2, _LC(".s")),
	_DECL_PGSQL_FUNC(exec, 2, _LC("xs")),
	_DECL_PGSQL_FUNC(query, 2, _LC("xs")),
	_DECL_PGSQL_FUNC(fetch, 1, _LC("x")),
	_DECL_PGSQL_FUNC(rows, 1, _LC("x")),
	_DECL_PGSQL_FUNC(version, 1, _LC("x")),
	_DECL_PGSQL_FUNC(database, 1, _LC("x")),
	_DECL_PGSQL_FUNC(user, 1, _LC("x")),
	_DECL_PGSQL_FUNC(_typeof, 1, _LC("x")),
	{NULL, (SQFUNCTION)0, 0, NULL}
};
#undef _DECL_PGSQL_FUNC

LVRESULT mod_init_pgsql(VMHANDLE v) {
	SQInteger i = 0;

	lv_pushstring(v, _LC("pgsql"), -1);
	lv_newclass(v, LVFalse);
	while (pgsqllib_funcs[i].name != 0) {
		lv_pushstring(v, pgsqllib_funcs[i].name, -1);
		lv_newclosure(v, pgsqllib_funcs[i].f, 0);
		lv_setparamscheck(v, pgsqllib_funcs[i].nparamscheck, pgsqllib_funcs[i].typemask);
		lv_setnativeclosurename(v, -1, pgsqllib_funcs[i].name);
		lv_newslot(v, -3, LVFalse);
		i++;
	}
	lv_newslot(v, -3, LVFalse);

	return LV_OK;
}
