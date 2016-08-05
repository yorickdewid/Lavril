#include <fcgi_stdio.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lavril.h>

#ifdef SQUNICODE

#define scvprintf vfwprintf
#else

#define scvprintf vfprintf
#endif

/* some of the HTTP variables we are interest in */
#define MAX_VARS 30
const char *vars[MAX_VARS] = {
    "DOCUMENT_ROOT",
    "GATEWAY_INTERFACE",
    "HTTP_ACCEPT",
    "HTTP_ACCEPT_ENCODING",
    "HTTP_ACCEPT_LANGUAGE",
    "HTTP_CACHE_CONTROL",
    "HTTP_CONNECTION",
    "HTTP_HOST",
    "HTTP_PRAGMA",
    "HTTP_RANGE",
    "HTTP_REFERER",
    "HTTP_TE",
    "HTTP_USER_AGENT",
    "HTTP_X_FORWARDED_FOR",
    "PATH",
    "QUERY_STRING",
    "REMOTE_ADDR",
    "REMOTE_HOST",
    "REMOTE_PORT",
    "REQUEST_METHOD",
    "REQUEST_URI",
    "SCRIPT_FILENAME",
    "SCRIPT_NAME",
    "SERVER_ADDR",
    "SERVER_ADMIN",
    "SERVER_NAME",
    "SERVER_PORT",
    "SERVER_PROTOCOL",
    "SERVER_SIGNATURE",
    "SERVER_SOFTWARE"
};

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

void call_test(HSQUIRRELVM v, int n, float f, const SQChar *s) {
	/* Save the stack size before the call */
	SQInteger top = lv_gettop(v);

	/* Push the global table */
	lv_pushroottable(v);
	lv_pushstring(v, _SC("test"), -1);

	/* Get the routine 'test' from the global table */
	if (LV_SUCCEEDED(lv_get(v, -2))) {
		lv_pushroottable(v);
		lv_pushinteger(v, n);
		lv_pushfloat(v, f);
		lv_pushstring(v, s, -1);
		lv_call(v, 4, SQFalse, SQTrue);
	}

	/* Restore the original stack size */
	lv_settop(v, top);
}

int main(int argc, char *argv[]) {
	HSQUIRRELVM v;

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

    while (FCGI_Accept() >= 0) {
		char *script = getenv("SCRIPT_NAME");
		if (!script) {
			printf("Status: 500\r\nContent-type: text/plain\r\n\r\n");
			puts("no script name provided");
			continue;
		}

		if (script[0] == '/')
			script++;

		printf("Content-type: text/plain\r\n\r\n");
		if (LV_FAILED(lv_execfile(v, script, SQFalse, SQTrue)))  {
			puts("an unknown error occurred");
		}
	}

	/* Pop the root table */
	lv_pop(v, 1);

	/* Release the VM */
	lv_close(v);

	return 0;
}

