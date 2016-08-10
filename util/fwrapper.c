#include <fcgi_stdio.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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

const char *error_body = "<html>"
                         "<head><title>%d %s</title></head>"
                         "<body bgcolor=\"white\">"
                         "<h1>%d %s</h1>"
                         "%s"
                         "<hr>lavril"
                         "</body>"
                         "</html>\r\n";

void print_error(int code, const char *status, const char *message) {
	scprintf(error_body, code, status, code, status, message ? message : "");
}

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

static SQInteger apprun(HSQUIRRELVM v) {
	const SQChar *s;
	if (LV_SUCCEEDED(lv_getstring(v, 2, &s))) {
		scprintf(_LC("%s\r\n"), s);
	}

	return 0;
}

void create_webapp_core(HSQUIRRELVM v) {
	lv_pushroottable(v);
	lv_pushstring(v, _LC("webapp"), -1);
	lv_newclass(v, SQFalse);

	lv_pushstring(v, _LC("run"), -1);
	lv_newclosure(v, apprun, 0);
	lv_setparamscheck(v, 2, ".s");
	lv_setnativeclosurename(v, -1, _LC("run"));
	lv_newslot(v, -3, SQFalse);

	lv_newslot(v, -3, SQFalse);
	lv_pop(v, 1);
}

int main(int argc, char *argv[]) {
	HSQUIRRELVM v = lv_open(1024);
	lv_setprintfunc(v, print_func, error_func);

	/* Push the roottable */
	lv_pushroottable(v);

	/* Load modules */
	init_module(blob, v);
	init_module(io, v);
	init_module(string, v);
	init_module(system, v);
	init_module(math, v);
	init_module(crypto, v);
	init_module(json, v);

	lv_registererrorhandlers(v);

	create_webapp_core(v);

	/* Accept requests */
	while (FCGI_Accept() >= 0) {
		char *script = getenv("SCRIPT_NAME");
		if (!script) {
			scprintf("Status: 500\r\nContent-type: text/html\r\n\r\n");
			print_error(500, "Internal Server Error", "No script name provided");
			continue;
		}

		if (script[0] == '/')
			script++;

		if (LV_FAILED(access(script, F_OK))) {
			scprintf("Status: 404\r\nContent-type: text/html\r\n\r\n");
			print_error(404, "Not Found", NULL);
			continue;
		}

		scprintf("Content-type: text/html\r\n\r\n");
		if (LV_FAILED(lv_execfile(v, script, SQFalse, SQTrue)))  {
			const SQChar *err;
			lv_getlasterror(v);
			if (LV_SUCCEEDED(lv_getstring(v, -1, &err))) {
				scprintf(_LC("<br /><b>An error occurred:</b> <pre>[%s]</pre>\r\n"), err);
				lv_reseterror(v);
			}
		}
	}

	/* Pop the root table */
	lv_pop(v, 1);

	/* Release the VM */
	lv_close(v);

	return 0;
}

