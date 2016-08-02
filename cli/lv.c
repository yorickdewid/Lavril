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

#define CLI_MAXINPUT 1024

void print_version_info();

enum result {
	DONE,
	ERROR,
	INTERACTIVE,
};

#if defined(_MSC_VER) && defined(_DEBUG)
int MemAllocHook(int allocType, void *userData, size_t size, int blockType,
                 long requestNumber, const unsigned char *filename, int lineNumber) {
	return 1;
}
#endif

SQInteger quit(HSQUIRRELVM v) {
	int *done;
	sq_getuserpointer(v, -1, (SQUserPointer *)&done);
	*done = 1;
	return 0;
}

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

void print_version_info() {
	scfprintf(stdout, _SC("%s %s (%d bits)\n"), LAVRIL_VERSION, LAVRIL_COPYRIGHT, ((int)(sizeof(SQInteger) * 8)));
}

void print_interactive_console_info() {
	scfprintf(stdout, _SC("Enter 'quit()' to exit\n"));
}

void print_usage() {
	scfprintf(stderr, _SC("usage: lv [options] <file> [--] [args...]\n\n")
	          _SC("  -a              Run interactively\n")
	          _SC("  -r <statement>  Run statement and exit\n")
	          _SC("  -c <file>       Compile file (default output 'out.lavc')\n")
	          _SC("  -o <file>       Specifies output file for the -c option\n")
	          _SC("  -d              Enable debug info\n")
	          _SC("  -v              Version\n")
	          _SC("  -h              This help\n"));
}

enum result getargs(HSQUIRRELVM v, int argc, char *argv[], SQInteger *retval) {
	int i;
	int compiles_only = 0;
	int run_statement_only = 0;
#ifdef SQUNICODE
	static SQChar temp[512];
#endif
	char *output = NULL;
	char *statement = NULL;
	*retval = 0;
	if (argc > 1) {
		int arg = 1, exitloop = 0;

		while (arg < argc && !exitloop) {

			if (argv[arg][0] == '-') {
				switch (argv[arg][1]) {
					case 'd':
						lv_enabledebuginfo(v, 1);
						break;
					case 'c':
						compiles_only = 1;
						break;
					case 'r':
						if (arg < argc) {
							arg++;
							statement = argv[arg];
							run_statement_only = 1;
						}
						break;
					case 'a':
						return INTERACTIVE;
					case 'o':
						if (arg < argc) {
							arg++;
							output = argv[arg];
						}
						break;
					case 'v':
						print_version_info();
						return DONE;

					case 'h':
						print_version_info();
						print_usage();
						return DONE;
					default:
						print_version_info();
						scprintf(_SC("unknown prameter '-%c'\n"), argv[arg][1]);
						print_usage();
						*retval = -1;
						return ERROR;
				}
			} else {
				break;
			}

			arg++;
		}

		if (run_statement_only) {
			SQInteger _retval = 0;

			int i = scstrlen(statement);
			if (i > 0) {
				SQInteger oldtop = lv_gettop(v);
				if (LV_SUCCEEDED(lv_compilebuffer(v, statement, i, _SC("lv"), SQTrue))) {
					sq_pushroottable(v);
					if (LV_SUCCEEDED(sq_call(v, 1, _retval, SQTrue)) && _retval) {
						scprintf(_SC("\n"));
						sq_pushroottable(v);
						sq_pushstring(v, _SC("print"), -1);
						sq_get(v, -2);
						sq_pushroottable(v);
						lv_push(v, -4);
						sq_call(v, 2, SQFalse, SQTrue);
						_retval = 0;
						scprintf(_SC("\n"));
					}
				}

				lv_settop(v, oldtop);
			}

			return DONE;
		}

		if (arg < argc) {
			const SQChar *filename = NULL;
#ifdef SQUNICODE
			mbstowcs(temp, argv[arg], strlen(argv[arg]));
			filename = temp;
#else
			filename = argv[arg];
#endif

			arg++;

			if (compiles_only) {
				if (LV_SUCCEEDED(lv_loadfile(v, filename, SQTrue))) {
					const SQChar *outfile = _SC("out.lavc");
					if (output) {
#ifdef SQUNICODE
						int len = (int)(strlen(output) + 1);
						mbstowcs(sq_getscratchpad(v, len * sizeof(SQChar)), output, len);
						outfile = sq_getscratchpad(v, -1);
#else
						outfile = output;
#endif
					}
					if (LV_SUCCEEDED(lv_writeclosuretofile(v, outfile))) {
						return DONE;
					}
				}
			} else {
				if (LV_SUCCEEDED(lv_loadfile(v, filename, SQTrue))) {
					int callargs = 1;
					sq_pushroottable(v);

					for (i = arg; i < argc; ++i) {
						const SQChar *a;
#ifdef SQUNICODE
						int alen = (int)strlen(argv[i]);
						a = sq_getscratchpad(v, (int)(alen * sizeof(SQChar)));
						mbstowcs(sq_getscratchpad(v, -1), argv[i], alen);
						sq_getscratchpad(v, -1)[alen] = _SC('\0');
#else
						a = argv[i];
#endif
						sq_pushstring(v, a, -1);
						callargs++;
					}

					if (LV_SUCCEEDED(sq_call(v, callargs, SQTrue, SQTrue))) {
						SQObjectType type = sq_gettype(v, -1);
						if (type == OT_INTEGER) {
							*retval = type;
							sq_getinteger(v, -1, retval);
						}
						return DONE;
					}

					return ERROR;
				}
			}

			/* If this point is reached an error occured */
			// {
			// 	const SQChar *err;
			// 	sq_getlasterror(v);
			// 	if (LV_SUCCEEDED(sq_getstring(v, -1, &err))) {
			// 		scprintf(_SC("Error [%s]\n"), err);
			// 		*retval = -2;
			// 		return ERROR;
			// 	}
			// }
		}
	}

	return INTERACTIVE;
}

void interactive(HSQUIRRELVM v) {
	SQChar buffer[CLI_MAXINPUT];
	SQInteger blocks = 0;
	SQInteger string = 0;
	SQInteger retval = 0;
	SQInteger done = 0;

	print_version_info();
	print_interactive_console_info();

	sq_pushroottable(v);
	sq_pushstring(v, _SC("quit"), -1);
	sq_pushuserpointer(v, &done);
	sq_newclosure(v, quit, 1);
	sq_setparamscheck(v, 1, NULL);
	sq_newslot(v, -3, SQFalse);
	lv_pop(v, 1);

	while (!done) {
		SQInteger i = 0;
		scprintf(_SC("\nlv> "));
		while (!done) {
			int c = getchar();
			if (c == _SC('\n')) {
				if (i > 0 && buffer[i - 1] == _SC('\\')) {
					buffer[i - 1] = _SC('\n');
				} else if (i > 0 && buffer[i - 1] == _SC('q') && buffer[i - 2] == _SC('\\')) {
					done = 1;
					i = 0;
					continue;
				} else if (blocks == 0) {
					break;
				}

				buffer[i++] = _SC('\n');
			} else if (c == _SC('}')) {
				blocks--;
				buffer[i++] = (SQChar)c;
			} else if (c == _SC('{') && !string) {
				blocks++;
				buffer[i++] = (SQChar)c;
			} else if (c == _SC('"') || c == _SC('\'')) {
				string = !string;
				buffer[i++] = (SQChar)c;
			} else if (i >= CLI_MAXINPUT - 1) {
				scfprintf(stderr, _SC("lv : input line too long\n"));
				break;
			} else {
				buffer[i++] = (SQChar)c;
			}
		}

		buffer[i] = _SC('\0');

		if (buffer[0] == _SC('=')) {
			scsprintf(sq_getscratchpad(v, CLI_MAXINPUT), (size_t)CLI_MAXINPUT, _SC("return (%s)"), &buffer[1]);
			memcpy(buffer, sq_getscratchpad(v, -1), (scstrlen(sq_getscratchpad(v, -1)) + 1)*sizeof(SQChar));
			retval = 1;
		}

		i = scstrlen(buffer);
		if (i > 0) {
			SQInteger oldtop = lv_gettop(v);
			if (LV_SUCCEEDED(lv_compilebuffer(v, buffer, i, _SC("lv"), SQTrue))) {
				sq_pushroottable(v);
				if (LV_SUCCEEDED(sq_call(v, 1, retval, SQTrue)) && retval) {
					scprintf(_SC("\n"));
					sq_pushroottable(v);
					sq_pushstring(v, _SC("print"), -1);
					sq_get(v, -2);
					sq_pushroottable(v);
					lv_push(v, -4);
					sq_call(v, 2, SQFalse, SQTrue);
					retval = 0;
					scprintf(_SC("\n"));
				}
			}

			lv_settop(v, oldtop);
		}
	}
}

int main(int argc, char *argv[]) {
	HSQUIRRELVM v;
	SQInteger retval = 0;

#if defined(_MSC_VER) && defined(_DEBUG)
	_CrtSetAllocHook(MemAllocHook);
#endif

	v = lv_open(1024);
	lv_setprintfunc(v, print_func, error_func);

	sq_pushroottable(v);

	init_module(blob, v);
	init_module(io, v);
	init_module(string, v);
	init_module(system, v);
	init_module(math, v);

	sq_registererrorhandlers(v);

	switch (getargs(v, argc, argv, &retval)) {
		case INTERACTIVE:
			interactive(v);
			break;
		case DONE:
		case ERROR:
		default:
			break;
	}

	lv_close(v);

#if defined(_MSC_VER) && defined(_DEBUG)
	_getch();
	_CrtMemDumpAllObjectsSince(NULL);
#endif

	return retval;
}
