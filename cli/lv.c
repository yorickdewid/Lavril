#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#include <conio.h>
#endif
#include <lavril.h>

#ifdef LVUNICODE
#define scfprintf fwprintf
#define scvprintf vfwprintf
#else
#define scfprintf fprintf
#define scvprintf vfprintf
#endif

#define CLI_MAXINPUT 1024
#define OPGEN_MIN 512

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

LVInteger quit(VMHANDLE v) {
	int *done;
	lv_getuserpointer(v, -1, (LVUserPointer *)&done);
	*done = 1;
	return 0;
}

void print_func(VMHANDLE LV_UNUSED_ARG(v), const LVChar *s, ...) {
	va_list vl;
	va_start(vl, s);
	scvprintf(stdout, s, vl);
	va_end(vl);
}

void error_func(VMHANDLE LV_UNUSED_ARG(v), const LVChar *s, ...) {
	va_list vl;
	va_start(vl, s);
	scvprintf(stderr, s, vl);
	va_end(vl);
}

LVChar *read_func(VMHANDLE LV_UNUSED_ARG(v)) {
	LVChar *line = lv_malloc(128);
	LVChar *linep = line;
	size_t lenmax = 128, len = lenmax;
	int c;

	if (!line)
		return NULL;

	for (;;) {
		c = fgetc(stdin);
		if (c == EOF)
			break;

		if (--len == 0) {
			len = lenmax;
			char *linen = lv_realloc(linep, 0, lenmax *= 2);

			if (!linen) {
				lv_free(linep, 0);
				return NULL;
			}
			line = linen + (line - linep);
			linep = linen;
		}

		if ((*line++ = c) == '\n')
			break;
	}
	*(line - 1) = '\0';
	return linep;
}

void print_version_info() {
	scfprintf(stdout, _LC("%s %s (%d bits)\n"), LAVRIL_VERSION, LAVRIL_COPYRIGHT, ((int)(sizeof(LVInteger) * 8)));
}

void print_interactive_console_info() {
	scfprintf(stdout, _LC("Enter 'quit()' to exit\n"));
}

void print_usage() {
	scfprintf(stderr, _LC("usage: lv [options] <file> [--] [args...]\n\n")
	          _LC("  -a              Run interactively\n")
	          _LC("  -r <statement>  Run statement and exit\n")
	          _LC("  -c <file>       Compile file (default output 'out.lavc')\n")
	          _LC("  -d              Enable debug info\n")
	          _LC("  -n              Always run script (omit compile cache)\n")
	          _LC("  -v              Version\n")
	          _LC("  -h              This help\n"));
}

time_t get_mtime(const char *path) {
	struct stat statbuf;
	if (stat(path, &statbuf) < 0) {
		return 0;
	}
	return statbuf.st_mtime;
}

off_t get_fsize(const char *path) {
	struct stat statbuf;
	if (stat(path, &statbuf) < 0) {
		return 0;
	}
	return statbuf.st_size;;
}

enum result getargs(VMHANDLE v, int argc, char *argv[], LVInteger *retval) {
	int i;
	int compiles_only = 0;
	int run_statement_only = 0;
	int omit_compile = 0;
#ifdef LVUNICODE
	static LVChar temp[512];
#endif
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
					case 'n':
						omit_compile = 1;
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
			LVInteger _retval = 0;

			int i = scstrlen(statement);
			if (i > 0) {
				LVInteger oldtop = lv_gettop(v);
				if (LV_SUCCEEDED(lv_compilebuffer(v, statement, i, _LC("lv"), LVTrue))) {
					lv_pushroottable(v);
					lv_call(v, 1, _retval, LVTrue);
				}

				lv_settop(v, oldtop);
			}

			return DONE;
		}

		if (arg < argc) {
			const LVChar *filename = NULL;
#ifdef LVUNICODE
			mbstowcs(temp, argv[arg], strlen(argv[arg]));
			filename = temp;
#else
			filename = argv[arg];
#endif

			arg++;

			size_t len = (strlen(filename) + 2);
			lv_getscratchpad(v, len * sizeof(LVChar));
			LVChar *outfile = lv_getscratchpad(v, -1);
			strcpy(outfile, filename);
			strcat(outfile, "c");

			if (compiles_only) {
				if (LV_SUCCEEDED(lv_loadfile(v, filename, LVTrue))) {
					if (LV_SUCCEEDED(lv_writeclosuretofile(v, outfile))) {
						return DONE;
					}
				}
			} else {
				if (!omit_compile) {
					time_t tcunit = get_mtime(outfile);
					time_t tunit = get_mtime(filename);

					if (tcunit) {
						if (tcunit > tunit) {
							filename = outfile;
						} else {
							if (LV_SUCCEEDED(lv_loadfile(v, filename, LVTrue))) {
								if (LV_SUCCEEDED(lv_writeclosuretofile(v, outfile))) {
									filename = outfile;
								}
							}
						}
					} else if (get_fsize(filename) > OPGEN_MIN) {
						if (LV_SUCCEEDED(lv_loadfile(v, filename, LVTrue))) {
							if (LV_SUCCEEDED(lv_writeclosuretofile(v, outfile))) {
								filename = outfile;
							}
						}
					}
				}

				if (LV_SUCCEEDED(lv_loadfile(v, filename, LVTrue))) {
					int callargs = 1;
					lv_pushroottable(v);

					for (i = arg; i < argc; ++i) {
						const LVChar *a;
#ifdef LVUNICODE
						int alen = (int)strlen(argv[i]);
						a = lv_getscratchpad(v, (int)(alen * sizeof(LVChar)));
						mbstowcs(lv_getscratchpad(v, -1), argv[i], alen);
						lv_getscratchpad(v, -1)[alen] = _LC('\0');
#else
						a = argv[i];
#endif
						lv_pushstring(v, a, -1);
						callargs++;
					}

					if (LV_SUCCEEDED(lv_call(v, callargs, LVTrue, LVTrue))) {
						LVObjectType type = lv_gettype(v, -1);
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
			return ERROR;
		}
	}

	return INTERACTIVE;
}

void interactive(VMHANDLE v) {
	LVChar buffer[CLI_MAXINPUT];
	LVInteger blocks = 0;
	LVInteger string = 0;
	LVInteger retval = 0;
	LVInteger done = 0;

	print_version_info();
	print_interactive_console_info();

	lv_pushroottable(v);
	lv_pushstring(v, _LC("quit"), -1);
	lv_pushuserpointer(v, &done);
	lv_newclosure(v, quit, 1);
	lv_setparamscheck(v, 1, NULL);
	lv_newslot(v, -3, LVFalse);
	lv_pop(v, 1);

	while (!done) {
		LVInteger i = 0;
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
				buffer[i++] = (LVChar)c;
			} else if (c == _LC('{') && !string) {
				blocks++;
				buffer[i++] = (LVChar)c;
			} else if (c == _LC('"') || c == _LC('\'')) {
				string = !string;
				buffer[i++] = (LVChar)c;
			} else if (i >= CLI_MAXINPUT - 1) {
				scfprintf(stderr, _LC("lv: input line too long\n"));
				break;
			} else {
				buffer[i++] = (LVChar)c;
			}
		}

		buffer[i] = _LC('\0');

		if (buffer[0] == _LC('=')) {
			scsprintf(lv_getscratchpad(v, CLI_MAXINPUT), (size_t)CLI_MAXINPUT, _LC("return (%s)"), &buffer[1]);
			memcpy(buffer, lv_getscratchpad(v, -1), (scstrlen(lv_getscratchpad(v, -1)) + 1)*sizeof(LVChar));
			retval = 1;
		}

		i = scstrlen(buffer);
		if (i > 0) {
			LVInteger oldtop = lv_gettop(v);
			if (LV_SUCCEEDED(lv_compilebuffer(v, buffer, i, _LC("lv"), LVTrue))) {
				lv_pushroottable(v);
				if (LV_SUCCEEDED(lv_call(v, 1, retval, LVTrue)) && retval) {
					lv_pushroottable(v);
					lv_pushstring(v, _LC("println"), -1);
					lv_get(v, -2);
					lv_pushroottable(v);
					lv_push(v, -4);
					lv_call(v, 2, LVFalse, LVTrue);
					retval = 0;
				}
			}

			lv_settop(v, oldtop);
		}
	}
}

int main(int argc, char *argv[]) {
	VMHANDLE v;
	LVInteger retval = 0;

#if defined(_MSC_VER) && defined(_DEBUG)
	_CrtSetAllocHook(MemAllocHook);
#endif

	v = lv_open(1024);

	lv_setprintfunc(v, print_func, error_func);
	lv_setreadfunc(v, read_func);

	lv_pushroottable(v);

	/* Load modules */
	lv_init_modules(v);

	init_module(sqlite, v);
	init_module(pgsql, v);

	lv_registererrorhandlers(v);

	srand(time(NULL));

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
