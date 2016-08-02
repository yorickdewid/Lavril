#include <stdarg.h>
#include <stdio.h>

#include <lavril.h>

#ifdef _MSC_VER
#pragma comment (lib ,"squirrel.lib")
#pragma comment (lib ,"sqstdlib.lib")
#endif

#ifdef SQUNICODE

#define scvprintf vfwprintf
#else

#define scvprintf vfprintf
#endif

void print_func(HSQUIRRELVM v, const SQChar *s, ...) {
	va_list vl;
	va_start(vl, s);
	scvprintf(stdout, s, vl);
	va_end(vl);
}

void error_func(HSQUIRRELVM v, const SQChar *s, ...) {
	va_list vl;
	va_start(vl, s);
	scvprintf(stderr, s, vl);
	va_end(vl);
}

void call_main(HSQUIRRELVM v, int n, float f, const SQChar *s) {
	/* Save the stack size before the call */
	SQInteger top = sq_gettop(v);

	/* Push the global table */
	sq_pushroottable(v);
	sq_pushstring(v, _SC("test"), -1);

	/* Get the routine 'test' from the global table */
	if (LV_SUCCEEDED(sq_get(v, -2))) {
		sq_pushroottable(v);
		sq_pushinteger(v, n);
		sq_pushfloat(v, f);
		sq_pushstring(v, s, -1);
		sq_call(v, 4, SQFalse, SQTrue);
	}

	/* Restore the original stack size */
	sq_settop(v, top);
}

int main(int argc, char *argv[]) {
	HSQUIRRELVM v;

	/* Creates a VM with initial stack size 1024 */
	v = sq_open(1024);

	{
		/* Push the global table to store std */
		sq_pushroottable(v);

		/* Load modules */
		init_module(math, v);

		/* Pop the root table */
		sq_pop(v, 1);
	}

	/* Registers the default error handlers */
	sq_registererrorhandlers(v);

	/* Sets the print function */
	sq_setprintfunc(v, print_func, error_func);

	/* Push the root table(were the globals of the script will be stored) */
	sq_pushroottable(v);

	/* Print syntax errors if any */
	if (LV_SUCCEEDED(sq_execfile(v, _SC("test.lav"), SQFalse, SQTrue)))  {
		call_main(v, 1, 2.5, _SC("teststring"));
	}

	/* Pop the root table */
	sq_pop(v, 1);

	/* Release the VM */
	sq_close(v);

	return 0;
}
