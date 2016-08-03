#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#include <conio.h>
#endif
#include <lavril.h>

#ifdef SQUNICODE
#define scfprintf fwprintf
#define scvprintf vfwprintf
#else
#define scfprintf fprintf
#define scvprintf vfprintf
#endif

void print_func(HSQUIRRELVM LV_UNUSED_ARG(v), const SQChar *s, ...) {
	va_list vl;
	va_start(vl, s);
	scvprintf(stdout, s, vl);
	va_end(vl);
}

void error_func(HSQUIRRELVM LV_UNUSED_ARG(v), const SQChar *s, ...) {
	va_list vl;
	va_start(vl, s);
	scvprintf(stderr, s, vl);
	va_end(vl);
}

const char *get_file_out(char *filename) {
	strcat(filename, "c");
	return filename;
}

int main(int argc, char *argv[]) {
	HSQUIRRELVM v;
	SQInteger retval = 0;

	v = lv_open(1024);
	lv_setprintfunc(v, print_func, error_func);

	lv_pushroottable(v);

	init_module(blob, v);
	init_module(io, v);
	init_module(string, v);
	init_module(system, v);
	init_module(math, v);
	init_module(crypto, v);

	lv_registererrorhandlers(v);

	if (LV_SUCCEEDED(lv_loadfile(v, argv[1], SQTrue))) {
		lv_writeclosuretofile(v, get_file_out(argv[1]));
	}

	lv_close(v);

	return retval;
}