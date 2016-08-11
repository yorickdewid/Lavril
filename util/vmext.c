#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <lavril.h>

#ifdef _MSC_VER
#pragma comment (lib ,"squirrel.lib")
#pragma comment (lib ,"sqstdlib.lib")
#endif

#ifdef LVUNICODE

#define scvprintf vfwprintf
#else

#define scvprintf vfprintf
#endif

void print_func(VMHANDLE v, const SQChar *s, ...) {
	va_list vl;
	va_start(vl, s);
	scvprintf(stdout, s, vl);
	va_end(vl);
}

void error_func(VMHANDLE v, const SQChar *s, ...) {
	va_list vl;
	va_start(vl, s);
	scvprintf(stderr, s, vl);
	va_end(vl);
}

// void call_test(VMHANDLE v, int n, float f, const SQChar *s) {
// 	/* Save the stack size before the call */
// 	SQInteger top = lv_gettop(v);

// 	/* Push the global table */
// 	lv_pushroottable(v);
// 	lv_pushstring(v, _LC("test"), -1);

// 	 Get the routine 'test' from the global table
// 	if (LV_SUCCEEDED(lv_get(v, -2))) {
// 		lv_pushroottable(v);
// 		lv_pushinteger(v, n);
// 		lv_pushfloat(v, f);
// 		lv_pushstring(v, s, -1);
// 		lv_call(v, 4, LVFalse, LVTrue);
// 	}

// 	/* Restore the original stack size */
// 	lv_settop(v, top);
// }

static SQInteger kaas(VMHANDLE v) {
	puts("kaas\n");
	return 0;
}

static SQInteger kaas_ex(VMHANDLE v) {
	puts("kaas_ex\n");
	return 0;
}

static SQInteger method(VMHANDLE v) {
	puts("method\n");
	return 0;
}

int main(int argc, char *argv[]) {
	VMHANDLE v;

	/* Creates a VM with initial stack size 1024 */
	v = lv_open(1024);

	{
		/* Push the global table to store std */
		// lv_pushroottable(v);

		/* Load modules */
		// init_module(math, v);

		/* Pop the root table */
		// lv_pop(v, 1);
	}

	/* Registers the default error handlers */
	lv_registererrorhandlers(v);

	/* Sets the print function */
	lv_setprintfunc(v, print_func, error_func);

	puts("FROM THIS POINT ON\n");

	/* Push roottable 1 */
	lv_pushroottable(v);


	// Function
	lv_pushstring(v, _LC("kaas"), -1);
	lv_newclosure(v, kaas, 0); //create a new function
	lv_newslot(v, -3, LVFalse);

	// Static var
	lv_pushstring(v, _LC("WOEI"), -1);
	lv_pushinteger(v, 17);
	lv_newslot(v, -3, LVFalse);

	// Function ex
	lv_pushstring(v, _LC("kaas_ex"), -1);
	lv_newclosure(v, kaas_ex, 0);
	lv_setparamscheck(v, 0, NULL);
	lv_setnativeclosurename(v, -1, _LC("kaas_ex"));
	lv_newslot(v, -3, LVFalse);

	// New table
	// lv_newtable(v);
	// lv_pushstring(v, _LC("sometable"), -1);
	// lv_pushinteger(v, 1);
	// lv_rawset(v, -3);

	// if (lv_gettype(v, -1) != OT_TABLE)
	// return lv_throwerror(v, _LC("table expected"));

	// SQInteger top = lv_gettop(v);

	//////////////////////////////
	lv_pushstring(v, _LC("troll"), -1);
	lv_newclass(v, LVFalse);

	lv_pushstring(v, _LC("method"), -1);
	lv_newclosure(v, method, 0);
	lv_setparamscheck(v, 0, NULL);
	lv_setnativeclosurename(v, -1, _LC("method"));
	lv_newslot(v, -3, LVFalse);
	lv_stackdump(v);

	lv_newslot(v, -3, LVFalse);
	lv_poptop(v);


	//////////////////////////////

#if 0
	//create delegate
	lv_pushregistrytable(v);
	lv_pushstring(v, _LC("std_stream"), -1);
	if (LV_FAILED(lv_get(v, -2))) {
		lv_pushstring(v, _LC("std_stream"), -1);
		lv_newclass(v, LVFalse);
		lv_settypetag(v, -1, (LVUserPointer)SQ_STREAM_TYPE_TAG);
		SQInteger i = 0;
		while (_stream_methods[i].name != 0) {
			const SQRegFunction& f = _stream_methods[i];
			lv_pushstring(v, f.name, -1);
			lv_newclosure(v, f.f, 0);
			lv_setparamscheck(v, f.nparamscheck, f.typemask);
			lv_newslot(v, -3, LVFalse);
			i++;
		}
		lv_newslot(v, -3, LVFalse);

		lv_pushroottable(v);
		lv_pushstring(v, _LC("stream"), -1);
		lv_pushstring(v, _LC("std_stream"), -1);
		lv_get(v, -4);
		lv_newslot(v, -3, LVFalse);
		lv_pop(v, 1);
	} else {
		lv_pop(v, 1); //result
	}
	lv_pop(v, 1);


	lv_pushregistrytable(v);
	lv_pushstring(v, _LC("ace_troll"), -1);
	lv_pushstring(v, _LC("std_stream"), -1);
	if (LV_SUCCEEDED(lv_get(v, -3))) {
		lv_newclass(v, LVTrue);
		lv_settypetag(v, -1, 1/*tag*/);

		// SQInteger i = 0;
		// while (methods[i].name != 0) {
		// const SQRegFunction& f = methods[i];
		lv_pushstring(v, "method", -1);
		lv_newclosure(v, method, 0);
		lv_setparamscheck(v, 0, NULL);
		lv_setnativeclosurename(v, -1, "method");
		lv_newslot(v, -3, LVFalse);
		// i++;
		// }

		lv_newslot(v, -3, LVFalse);
		lv_pop(v, 1); // pop registry

		// i = 0;
		// while (globals[i].name != 0) {
		// const SQRegFunction& f = globals[i];

		// lv_pushstring(v, f.name, -1);
		// lv_newclosure(v, f.f, 0);
		// lv_setparamscheck(v, f.nparamscheck, f.typemask);
		// lv_setnativeclosurename(v, -1, f.name);
		// lv_newslot(v, -3, LVFalse);

		// i++;
		// }
		//register the class in the target table
		lv_pushstring(v, _LC("troll"), -1);
		lv_pushregistrytable(v);
		lv_pushstring(v, _LC("ace_troll"), -1);
		lv_get(v, -2);
		lv_remove(v, -2);
		lv_newslot(v, -3, LVFalse);

		// lv_settop(v, top);
		// return LV_OK;
		// }
		lv_settop(v, top);
		// return LV_ERROR;
	}
#endif



	/* Pop roottable 1 */
	// lv_pop(v, 1);

	const char *statement = "var _x = troll();println(_x.method());";

	SQInteger _retval = 0;
	SQInteger sz = scstrlen(statement);
	if (LV_SUCCEEDED(lv_compilebuffer(v, statement, sz, _LC("vmext"), LVTrue))) {
		lv_pushroottable(v);
		if (LV_SUCCEEDED(lv_call(v, 1, _retval, LVTrue)) && _retval) {
			// scprintf(_LC("\n"));
			// lv_pushroottable(v);
			// lv_pushstring(v, _LC("print"), -1);
			// lv_get(v, -2);
			// lv_pushroottable(v);
			// lv_push(v, -4);
			// lv_call(v, 2, LVFalse, LVTrue);
			// _retval = 0;
			// scprintf(_LC("\n"));
		}
	}

	/* Release the VM */
	lv_close(v);

	return 0;
}
