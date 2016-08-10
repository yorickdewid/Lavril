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

SQInteger quit(VMHANDLE v) {
	int *done;
	lv_getuserpointer(v, -1, (SQUserPointer *)&done);
	*done = 1;
	return 0;
}

void print_func(VMHANDLE LV_UNUSED_ARG(v), const SQChar *s, ...) {
	va_list vl;
	va_start(vl, s);
	scvprintf(stdout, s, vl);
	va_end(vl);
}

void error_func(VMHANDLE LV_UNUSED_ARG(v), const SQChar *s, ...) {
	va_list vl;
	va_start(vl, s);
	scvprintf(stderr, s, vl);
	va_end(vl);
}

void print_version_info() {
	scfprintf(stdout, _LC("%s %s (%d bits)\n"), LAVRIL_VERSION, LAVRIL_COPYRIGHT, ((int)(sizeof(SQInteger) * 8)));
}

void print_interactive_console_info() {
	scfprintf(stdout, _LC("Enter 'quit()' to exit\n"));
}

void print_usage() {
	scfprintf(stderr, _LC("usage: lv [options] <file> [--] [args...]\n\n")
	          _LC("  -a              Run interactively\n")
	          _LC("  -r <statement>  Run statement and exit\n")
	          _LC("  -c <file>       Compile file (default output 'out.lavc')\n")
	          _LC("  -o <file>       Specifies output file for the -c option\n")
	          _LC("  -d              Enable debug info\n")
	          _LC("  -v              Version\n")
	          _LC("  -h              This help\n"));
}

enum result getargs(VMHANDLE v, int argc, char *argv[], SQInteger *retval) {
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
						scprintf(_LC("unknown prameter '-%c'\n"), argv[arg][1]);
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
				if (LV_SUCCEEDED(lv_compilebuffer(v, statement, i, _LC("lv"), SQTrue))) {
					lv_pushroottable(v);
					if (LV_SUCCEEDED(lv_call(v, 1, _retval, SQTrue)) && _retval) {
						scprintf(_LC("\n"));
						lv_pushroottable(v);
						lv_pushstring(v, _LC("print"), -1);
						lv_get(v, -2);
						lv_pushroottable(v);
						lv_push(v, -4);
						lv_call(v, 2, SQFalse, SQTrue);
						_retval = 0;
						scprintf(_LC("\n"));
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
					const SQChar *outfile = _LC("out.lavc");
					if (output) {
#ifdef SQUNICODE
						int len = (int)(strlen(output) + 1);
						mbstowcs(lv_getscratchpad(v, len * sizeof(SQChar)), output, len);
						outfile = lv_getscratchpad(v, -1);
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
					lv_pushroottable(v);

					for (i = arg; i < argc; ++i) {
						const SQChar *a;
#ifdef SQUNICODE
						int alen = (int)strlen(argv[i]);
						a = lv_getscratchpad(v, (int)(alen * sizeof(SQChar)));
						mbstowcs(lv_getscratchpad(v, -1), argv[i], alen);
						lv_getscratchpad(v, -1)[alen] = _LC('\0');
#else
						a = argv[i];
#endif
						lv_pushstring(v, a, -1);
						callargs++;
					}

					if (LV_SUCCEEDED(lv_call(v, callargs, SQTrue, SQTrue))) {
						SQObjectType type = lv_gettype(v, -1);
						if (type == OT_INTEGER) {
							*retval = type;
							lv_getinteger(v, -1, retval);
						}
						return DONE;
					}

					return ERROR;
				}
			}

			/* If this point is reached an error occured */
			{
				// const SQChar *err;
				// lv_getlasterror(v);
				// if (LV_SUCCEEDED(lv_getstring(v, -1, &err))) {
				// 	scprintf(_LC("Error [%s]\n"), err);
				// 	*retval = -2;
				return ERROR;
				// }
			}
		}
	}

	return INTERACTIVE;
}

void interactive(VMHANDLE v) {
	SQChar buffer[CLI_MAXINPUT];
	SQInteger blocks = 0;
	SQInteger string = 0;
	SQInteger retval = 0;
	SQInteger done = 0;

	print_version_info();
	print_interactive_console_info();

	lv_pushroottable(v);
	lv_pushstring(v, _LC("quit"), -1);
	lv_pushuserpointer(v, &done);
	lv_newclosure(v, quit, 1);
	lv_setparamscheck(v, 1, NULL);
	lv_newslot(v, -3, SQFalse);
	lv_pop(v, 1);

	while (!done) {
		SQInteger i = 0;
		scprintf(_LC("\nlv> "));
		while (!done) {
			int c = getchar();
			if (c == _LC('\n')) {
				if (i > 0 && buffer[i - 1] == _LC('\\')) {
					buffer[i - 1] = _LC('\n');
				} else if (i > 0 && buffer[i - 1] == _LC('q') && buffer[i - 2] == _LC('\\')) {
					done = 1;
					i = 0;
					continue;
				} else if (blocks == 0) {
					break;
				}

				buffer[i++] = _LC('\n');
			} else if (c == _LC('}')) {
				blocks--;
				buffer[i++] = (SQChar)c;
			} else if (c == _LC('{') && !string) {
				blocks++;
				buffer[i++] = (SQChar)c;
			} else if (c == _LC('"') || c == _LC('\'')) {
				string = !string;
				buffer[i++] = (SQChar)c;
			} else if (i >= CLI_MAXINPUT - 1) {
				scfprintf(stderr, _LC("lv : input line too long\n"));
				break;
			} else {
				buffer[i++] = (SQChar)c;
			}
		}

		buffer[i] = _LC('\0');

		if (buffer[0] == _LC('=')) {
			scsprintf(lv_getscratchpad(v, CLI_MAXINPUT), (size_t)CLI_MAXINPUT, _LC("return (%s)"), &buffer[1]);
			memcpy(buffer, lv_getscratchpad(v, -1), (scstrlen(lv_getscratchpad(v, -1)) + 1)*sizeof(SQChar));
			retval = 1;
		}

		i = scstrlen(buffer);
		if (i > 0) {
			SQInteger oldtop = lv_gettop(v);
			if (LV_SUCCEEDED(lv_compilebuffer(v, buffer, i, _LC("lv"), SQTrue))) {
				lv_pushroottable(v);
				if (LV_SUCCEEDED(lv_call(v, 1, retval, SQTrue)) && retval) {
					lv_pushroottable(v);
					lv_pushstring(v, _LC("println"), -1);
					lv_get(v, -2);
					lv_pushroottable(v);
					lv_push(v, -4);
					lv_call(v, 2, SQFalse, SQTrue);
					retval = 0;
				}
			}

			lv_settop(v, oldtop);
		}
	}
}

int main(int argc, char *argv[]) {
	VMHANDLE v;
	SQInteger retval = 0;

#if defined(_MSC_VER) && defined(_DEBUG)
	_CrtSetAllocHook(MemAllocHook);
#endif

	v = lv_open(1024);
	lv_setprintfunc(v, print_func, error_func);

	lv_pushroottable(v);

	init_module(blob, v);
	init_module(io, v);
	init_module(string, v);
	init_module(system, v);
	init_module(math, v);
	init_module(crypto, v);
	init_module(curl, v);
	init_module(json, v);

	lv_registererrorhandlers(v);

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
