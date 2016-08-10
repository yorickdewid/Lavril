/*
 * -------------------------------  lvsh.c  ---------------------------------
 *
 * Copyright (c) 2015-2016, Yorick de Wid <yorick17 at outlook dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * This shell acts an an template to easily test a program by adding some
 * commands to the right sections. These so called local commands are
 * executed first, but when no local command is found the input is forked
 * and executed as a terminal program. The shell implementation was written
 * by Anders H at https://gist.github.com/parse/966049
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <ctype.h>
// #include <sys/types.h>
#include <sys/wait.h>

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

#define READ  0
#define WRITE 1

enum Color {
	COLOR_RED,
	COLOR_GREEN,
	COLOR_YELLOW,
	COLOR_BLUE,
	COLOR_MAGENTA,
	COLOR_CYAN,
	COLOR_WHITE,
};

#define defcolor(escape) { \
	fprintf(stream, "\033[" #escape ";1m"); \
	va_start(valist, fmt); \
	scvprintf(stream, fmt, valist); \
	va_end(valist); \
	fprintf(stream, "\033[0m"); \
}

void color_fprintf(FILE *stream, enum Color color, const char *fmt, ...) {
	va_list valist;

	/* File output */
	if (!isatty(fileno(stream))) {
		va_start(valist, fmt);
		scvprintf(stream, fmt, valist);
		va_end(valist);
		return;
	}

	/* Print to console with color */
	switch (color) {
		case COLOR_RED:
			defcolor(31);
			break;
		case COLOR_GREEN:
			defcolor(32);
			break;
		case COLOR_YELLOW:
			defcolor(33);
			break;
		case COLOR_BLUE:
			defcolor(34);
			break;
		case COLOR_MAGENTA:
			defcolor(35);
			break;
		case COLOR_CYAN:
			defcolor(36);
			break;
		case COLOR_WHITE:
			defcolor(0);
			break;
		default:
			break;
	}
}

void print_version_info();
static SQInteger retval = 0;

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
	fprintf(stderr, "\033[31;1m");
	va_start(vl, s);
	scvprintf(stderr, s, vl);
	va_end(vl);
	fprintf(stderr, "\033[0m");
}

void print_version_info() {
	scfprintf(stdout, _LC("%s %s (%d bits)\n"), LAVRIL_VERSION, LAVRIL_COPYRIGHT, ((int)(sizeof(SQInteger) * 8)));
}

void print_interactive_console_info() {
	scfprintf(stdout, _LC("Enter 'quit()' to exit\n"));
}

char *args[512];
int n = 0;

#if 1
static int runcmd(short input, short first, short last) {
	int pipettes[2];

	/* Invoke pipe */
	pipe(pipettes);

	pid_t pid = fork();
	if (pid == 0) {
		if (first == 1 && last == 0 && input == 0) {
			/* First command */
			dup2(pipettes[WRITE], STDOUT_FILENO);
		} else if (first == 0 && last == 0 && input != 0) {
			/* Middle command */
			dup2(input, STDIN_FILENO);
			dup2(pipettes[WRITE], STDOUT_FILENO);
		} else {
			/* Last command */
			dup2(input, STDIN_FILENO);
		}

		if (execvp(args[0], args) == -1)
			_exit(EXIT_FAILURE); // If child fails
	}

	if (input != 0)
		close(input);

	// Nothing more needs to be written
	close(pipettes[WRITE]);

	// If it's the last command, nothing more needs to be read
	if (last == 1)
		close(pipettes[READ]);

	return pipettes[READ];
}
#endif

static void cleanup(int n) {
	int i;
	for (i = 0; i < n; ++i)
		wait(NULL);
}

char *skipwhite(char *s) {
	while (isspace(*s))
		++s;
	return s;
}

int split(char *cmd) {
	cmd = skipwhite(cmd);
	char *next = strchr(cmd, ' ');
	int i = 0;

	while (next != NULL) {
		next[0] = '\0';
		args[i] = cmd;
		++i;
		cmd = skipwhite(next + 1);
		next = strchr(cmd, ' ');
	}

	// if (cmd[0] != '\0') {
	// 	args[i] = cmd;
	// 	next = strchr(cmd, '\n');
	// 	next[0] = '\0';
	// 	++i;
	// }

	args[i] = NULL;
	return i;
}

int extrun(char *cmd, short input, short first, short last) {
	split(cmd);
	if (args[0] != NULL) {
		n += 1;
		return runcmd(input, first, last);
	}
	return 0;
}

void print_prompt() {
	char loginbuf[64];
	char hostnamebuf[1024];

	color_fprintf(stdout, COLOR_WHITE, _LC("["));

	if (!getlogin_r(loginbuf, sizeof(loginbuf))) {
		loginbuf[sizeof(loginbuf) - 1] = '\0';

		color_fprintf(stdout, COLOR_YELLOW, loginbuf);
		color_fprintf(stdout, COLOR_WHITE, _LC("@"));
	}

	if (!gethostname(hostnamebuf, sizeof(hostnamebuf))) {
		hostnamebuf[sizeof(hostnamebuf) - 1] = '\0';

		color_fprintf(stdout, COLOR_CYAN, hostnamebuf);
	}

	color_fprintf(stdout, COLOR_WHITE, _LC(":"));
	color_fprintf(stdout, COLOR_RED, _LC("%lld"), retval);
	color_fprintf(stdout, COLOR_WHITE, _LC("]$ "));
}

void interactive(VMHANDLE v) {
	SQChar buffer[CLI_MAXINPUT];
	SQInteger blocks = 0;
	SQInteger string = 0;
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
		print_prompt();
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

		if (buffer[0] == _LC('>')) {
			// puts(buffer);

			short input = 0;
			short first = 1;

			char *cmd = (buffer + 1);
			char *next = strchr(cmd, '|');
			while (next != NULL) {
				/* 'next' points to '|' */
				*next = '\0';
				input = extrun(cmd, input, first, 0);

				cmd = next + 1;
				next = strchr(cmd, '|'); /* Find next '|' */
				first = 0;
			}
			input = extrun(cmd, input, first, 1);
			cleanup(n);

			i = 0;
			continue;
		}

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
					// retval = 0;
				}
			}

			lv_settop(v, oldtop);
		}
	}
}

int main(int argc, char *argv[]) {
	SQInteger retval = 0;

	VMHANDLE v = lv_open(1024);
	lv_setprintfunc(v, print_func, error_func);

	lv_pushroottable(v);

	/* Initialize modules */
	init_module(blob, v);
	init_module(io, v);
	init_module(string, v);
	init_module(system, v);
	init_module(math, v);
	init_module(crypto, v);
	// init_module(curl, v);
	init_module(json, v);

	lv_registererrorhandlers(v);

	interactive(v);

	lv_close(v);

	return retval;
}
