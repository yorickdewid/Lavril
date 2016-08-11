#include <stdarg.h>
#include <stdio.h>

#include <lavril.h>

#ifdef _MSC_VER
#pragma comment (lib ,"lvcore.lib")
#pragma comment (lib ,"lvmods.lib")
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

void call_test(VMHANDLE v, int n, float f, const SQChar *s) {
	/* Save the stack size before the call */
	SQInteger top = lv_gettop(v);

	/* Push the global table */
	lv_pushroottable(v);
	lv_pushstring(v, _LC("test"), -1);

	/* Get the routine 'test' from the global table */
	if (LV_SUCCEEDED(lv_get(v, -2))) {
		lv_pushroottable(v);
		lv_pushinteger(v, n);
		lv_pushfloat(v, f);
		lv_pushstring(v, s, -1);
		lv_call(v, 4, LVFalse, LVTrue);
	}

	/* Restore the original stack size */
	lv_settop(v, top);
}

int main(int argc, char *argv[]) {
	VMHANDLE v;

	/* Creates a VM with initial stack size 1024 */
	v = lv_open(1024);

	{
		/* Push the global table to store std */
		lv_pushroottable(v);

		/* Load modules */
		init_module(math, v);

		/* Pop the root table */
		lv_pop(v, 1);
	}

	/* Registers the default error handlers */
	lv_registererrorhandlers(v);

	/* Sets the print function */
	lv_setprintfunc(v, print_func, error_func);

	/* Push the root table(were the globals of the script will be stored) */
	lv_pushroottable(v);

	/* Print syntax errors if any */
	if (LV_SUCCEEDED(lv_execfile(v, _LC("test.lav"), LVFalse, LVTrue)))  {
		call_test(v, 1, 2.5, _LC("teststring"));
	}

	/* Pop the root table */
	lv_pop(v, 1);

	/* Release the VM */
	lv_close(v);

	return 0;
}
