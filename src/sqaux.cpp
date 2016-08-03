#include "sqpcheader.h"

CALLBACK static void printcallstack(HSQUIRRELVM v) {
	SQPRINTFUNCTION pf = lv_geterrorfunc(v);
	if (pf) {
		SQStackInfos si;
		SQInteger level = 1; /* Skip native function */
		pf(v, _SC("\nBacktrace: (most recent first)\n"));
		while (LV_SUCCEEDED(lv_stackinfos(v, level, &si))) {
			const SQChar *fn = _SC("unknown");
			const SQChar *src = _SC("unknown");
			if (si.funcname)
				fn = si.funcname;
			if (si.source)
				src = si.source;
			pf(v, _SC("  [%d] %s:%s():%d\n"), level - 1, src, fn, si.line);
			level++;
		}

#ifdef _DEBUG_DUMP
		SQInteger i;
		SQFloat f;
		SQInteger seq = 0;
		const SQChar *name = 0;
		const SQChar *s;
		level = 0;
		pf(v, _SC("\nLOCALS\n"));

		for (level = 0; level < 10; level++) {
			seq = 0;
			while ((name = sq_getlocal(v, level, seq))) {
				seq++;
				switch (sq_gettype(v, -1)) {
					case OT_NULL:
						pf(v, _SC("[%s] NULL\n"), name);
						break;
					case OT_INTEGER:
						sq_getinteger(v, -1, &i);
						pf(v, _SC("[%s] %d\n"), name, i);
						break;
					case OT_FLOAT:
						sq_getfloat(v, -1, &f);
						pf(v, _SC("[%s] %.14g\n"), name, f);
						break;
					case OT_USERPOINTER:
						pf(v, _SC("[%s] USERPOINTER\n"), name);
						break;
					case OT_STRING:
						lv_getstring(v, -1, &s);
						pf(v, _SC("[%s] \"%s\"\n"), name, s);
						break;
					case OT_TABLE:
						pf(v, _SC("[%s] TABLE\n"), name);
						break;
					case OT_ARRAY:
						pf(v, _SC("[%s] ARRAY\n"), name);
						break;
					case OT_CLOSURE:
						pf(v, _SC("[%s] CLOSURE\n"), name);
						break;
					case OT_NATIVECLOSURE:
						pf(v, _SC("[%s] NATIVECLOSURE\n"), name);
						break;
					case OT_GENERATOR:
						pf(v, _SC("[%s] GENERATOR\n"), name);
						break;
					case OT_USERDATA:
						pf(v, _SC("[%s] USERDATA\n"), name);
						break;
					case OT_THREAD:
						pf(v, _SC("[%s] THREAD\n"), name);
						break;
					case OT_CLASS:
						pf(v, _SC("[%s] CLASS\n"), name);
						break;
					case OT_INSTANCE:
						pf(v, _SC("[%s] INSTANCE\n"), name);
						break;
					case OT_WEAKREF:
						pf(v, _SC("[%s] WEAKREF\n"), name);
						break;
					case OT_BOOL: {
						SQBool bval;
						sq_getbool(v, -1, &bval);
						pf(v, _SC("[%s] %s\n"), name, bval == SQTrue ? _SC("true") : _SC("false"));
					}
					break;
					default:
						assert(0);
						break;
				}
				sq_pop(v, 1);
			}
		}
#endif
	}
}

CALLBACK static SQInteger callback_printerror(HSQUIRRELVM v) {
	SQPRINTFUNCTION pf = lv_geterrorfunc(v);
	if (pf) {
		const SQChar *sErr = 0;
		if (lv_gettop(v) >= 1) {
			if (LV_SUCCEEDED(lv_getstring(v, 2, &sErr)))   {
				pf(v, _SC("fatal error: %s\n"), sErr);
			} else {
				pf(v, _SC("fatal error: [unknown]\n"));
			}
			printcallstack(v);
		}
	}
	return 0;
}

CALLBACK void callback_compiler_error(HSQUIRRELVM v, const SQChar *sErr, const SQChar *sSource, SQInteger line, SQInteger column) {
	SQPRINTFUNCTION pf = lv_geterrorfunc(v);
	if (pf) {
		pf(v, _SC("%s:%d:%d: error %s\n"), sSource, line, column, sErr);
	}
}

void lv_registererrorhandlers(HSQUIRRELVM v) {
	lv_setcompilererrorhandler(v, callback_compiler_error);
	lv_newclosure(v, callback_printerror, 0);
	lv_seterrorhandler(v);
}
