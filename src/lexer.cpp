#include "pcheader.h"
#include <ctype.h>
#include <stdlib.h>
#include "table.h"
#include "lvstring.h"
#include "compiler.h"
#include "lexer.h"

#define CUR_CHAR (_currdata)
#define RETURN_TOKEN(t) { _prevtoken = _curtoken; _curtoken = t; return t;}
#define IS_EOB() (CUR_CHAR <= SQUIRREL_EOB)
#define NEXT() {Next();_currentcolumn++;}
#define INIT_TEMP_STRING() { _longstr.resize(0);}
#define APPEND_CHAR(c) { _longstr.push_back(c);}
#define TERMINATE_BUFFER() {_longstr.push_back(_LC('\0'));}
#define ADD_KEYWORD(key,id) _keywords->NewSlot(SQString::Create(ss, _LC(#key)), SQInteger(id))

LVLexer::LVLexer() {}

LVLexer::~LVLexer() {
	_keywords->Release();
}

void LVLexer::Init(SQSharedState *ss, SQLEXREADFUNC rg, SQUserPointer up, CompilerErrorFunc efunc, void *ed) {
	_errfunc = efunc;
	_errtarget = ed;
	_sharedstate = ss;
	_keywords = SQTable::Create(ss, 38);

	ADD_KEYWORD(while, TK_WHILE);
	ADD_KEYWORD(do, TK_DO);
	ADD_KEYWORD(if, TK_IF);
	ADD_KEYWORD(else, TK_ELSE);
	ADD_KEYWORD(break, TK_BREAK);
	ADD_KEYWORD(continue, TK_CONTINUE);
	ADD_KEYWORD(return, TK_RETURN);
	ADD_KEYWORD(null, TK_NULL);
	ADD_KEYWORD(function, TK_FUNCTION);
	ADD_KEYWORD(var, TK_LOCAL);
	ADD_KEYWORD(for, TK_FOR);
	ADD_KEYWORD(foreach, TK_FOREACH);
	ADD_KEYWORD(in, TK_IN);
	ADD_KEYWORD(typeof, TK_TYPEOF);
	ADD_KEYWORD(base, TK_BASE);
	ADD_KEYWORD(delete, TK_DELETE);
	ADD_KEYWORD(try, TK_TRY);
	ADD_KEYWORD(catch, TK_CATCH);
	ADD_KEYWORD(throw, TK_THROW);
	ADD_KEYWORD(clone, TK_CLONE);
	ADD_KEYWORD(yield, TK_YIELD);
	ADD_KEYWORD(resume, TK_RESUME);
	ADD_KEYWORD(switch, TK_SWITCH);
	ADD_KEYWORD(case, TK_CASE);
	ADD_KEYWORD(default, TK_DEFAULT);
	ADD_KEYWORD(this, TK_THIS);
	ADD_KEYWORD(class, TK_CLASS);
	ADD_KEYWORD(extends, TK_EXTENDS);
	ADD_KEYWORD(constructor, TK_CONSTRUCTOR);
	ADD_KEYWORD(instanceof, TK_INSTANCEOF);
	ADD_KEYWORD(true, TK_TRUE);
	ADD_KEYWORD(false, TK_FALSE);
	ADD_KEYWORD(static, TK_STATIC);
	ADD_KEYWORD(enum, TK_ENUM);
	ADD_KEYWORD(const, TK_CONST);
	ADD_KEYWORD(include, TK_INCLUDE);
	ADD_KEYWORD(__LINE__, TK___LINE__);
	ADD_KEYWORD(__FILE__, TK___FILE__);

	_readf = rg;
	_up = up;
	_lasttokenline = _currentline = 1;
	_currentcolumn = 0;
	_prevtoken = -1;
	_reached_eof = LVFalse;
	Next();
}

void LVLexer::Error(const SQChar *err) {
	_errfunc(_errtarget, err);
}

void LVLexer::Next() {
	SQInteger t = _readf(_up);
	if (t > MAX_CHAR)
		Error(_LC("Invalid character"));

	if (t != 0) {
		_currdata = (LexChar)t;
		return;
	}
	_currdata = SQUIRREL_EOB;
	_reached_eof = LVTrue;
}

const SQChar *LVLexer::Tok2Str(SQInteger tok) {
	SQObjectPtr itr, key, val;
	SQInteger nitr;
	while ((nitr = _keywords->Next(false, itr, key, val)) != -1) {
		itr = (SQInteger)nitr;
		if (((SQInteger)_integer(val)) == tok)
			return _stringval(key);
	}
	return NULL;
}

void LVLexer::LexBlockComment() {
	bool done = false;
	while (!done) {
		switch (CUR_CHAR) {
			case _LC('*'): {
				NEXT();
				if (CUR_CHAR == _LC('/')) {
					done = true;
					NEXT();
				}
			};
			continue;
			case _LC('\n'):
				_currentline++;
				NEXT();
				continue;
			case SQUIRREL_EOB:
				Error(_LC("missing \"*/\" in comment"));
			default:
				NEXT();
		}
	}
}
void LVLexer::LexLineComment() {
	do {
		NEXT();
	} while (CUR_CHAR != _LC('\n') && (!IS_EOB()));
}

SQInteger LVLexer::Lex() {
	_lasttokenline = _currentline;
	while (CUR_CHAR != SQUIRREL_EOB) {
		switch (CUR_CHAR) {
			case _LC('\t'):
			case _LC('\r'):
			case _LC(' '):
				NEXT();
				continue;
			case _LC('\n'):
				_currentline++;
				_prevtoken = _curtoken;
				_curtoken = _LC('\n');
				NEXT();
				_currentcolumn = 1;
				continue;
			case _LC('#'):
				LexLineComment();
				continue;
			case _LC('/'):
				NEXT();
				switch (CUR_CHAR) {
					case _LC('*'):
						NEXT();
						LexBlockComment();
						continue;
					case _LC('/'):
						LexLineComment();
						continue;
					case _LC('='):
						NEXT();
						RETURN_TOKEN(TK_DIVEQ);
						continue;
						// case _LC('>'):
						// NEXT();
						// RETURN_TOKEN(TK_ATTR_CLOSE);
						continue;
					default:
						RETURN_TOKEN('/');
				}
			case _LC('='):
				NEXT();
				if (CUR_CHAR != _LC('=')) {
					RETURN_TOKEN('=')
				} else {
					NEXT();
					RETURN_TOKEN(TK_EQ);
				}
			case _LC('<'):
				NEXT();
				switch (CUR_CHAR) {
					case _LC('='):
						NEXT();
						if (CUR_CHAR == _LC('>')) {
							NEXT();
							RETURN_TOKEN(TK_3WAYSCMP);
						}
						RETURN_TOKEN(TK_LE)
						break;
					case _LC('-'):
						NEXT();
						RETURN_TOKEN(TK_NEWSLOT);
						break;
					case _LC('<'):
						NEXT();
						RETURN_TOKEN(TK_SHIFTL);
						break;
						// case _LC('/'):
						// NEXT();
						// RETURN_TOKEN(TK_ATTR_OPEN);
						break;
				}
				RETURN_TOKEN('<');
			case _LC('>'):
				NEXT();
				if (CUR_CHAR == _LC('=')) {
					NEXT();
					RETURN_TOKEN(TK_GE);
				} else if (CUR_CHAR == _LC('>')) {
					NEXT();
					if (CUR_CHAR == _LC('>')) {
						NEXT();
						RETURN_TOKEN(TK_USHIFTR);
					}
					RETURN_TOKEN(TK_SHIFTR);
				} else {
					RETURN_TOKEN('>')
				}
			case _LC('!'):
				NEXT();
				if (CUR_CHAR != _LC('=')) {
					RETURN_TOKEN('!')
				} else {
					NEXT();
					RETURN_TOKEN(TK_NE);
				}
			case _LC('@'): {
				SQInteger stype;
				NEXT();
				if (CUR_CHAR != _LC('"')) {
					RETURN_TOKEN('@');
				}
				if ((stype = ReadString('"', true)) != -1) {
					RETURN_TOKEN(stype);
				}
				Error(_LC("error parsing the string"));
			}
			case _LC('"'):
			case _LC('\''): {
				SQInteger stype;
				if ((stype = ReadString(CUR_CHAR, false)) != -1) {
					RETURN_TOKEN(stype);
				}
				Error(_LC("error parsing the string"));
			}
			case _LC('{'):
			case _LC('}'):
			case _LC('('):
			case _LC(')'):
			case _LC('['):
			case _LC(']'):
			case _LC(';'):
			case _LC(','):
			case _LC('?'):
			case _LC('^'):
			case _LC('~'): {
				SQInteger ret = CUR_CHAR;
				NEXT();
				RETURN_TOKEN(ret);
			}
			case _LC('.'):
				NEXT();
				if (CUR_CHAR != _LC('.')) {
					RETURN_TOKEN('.')
				}
				NEXT();
				if (CUR_CHAR != _LC('.')) {
					Error(_LC("invalid token '..'"));
				}
				NEXT();
				RETURN_TOKEN(TK_VARPARAMS);
			case _LC('&'):
				NEXT();
				if (CUR_CHAR != _LC('&')) {
					RETURN_TOKEN('&')
				} else {
					NEXT();
					RETURN_TOKEN(TK_AND);
				}
			case _LC('|'):
				NEXT();
				if (CUR_CHAR != _LC('|')) {
					RETURN_TOKEN('|')
				} else {
					NEXT();
					RETURN_TOKEN(TK_OR);
				}
			case _LC(':'):
				NEXT();
				if (CUR_CHAR != _LC(':')) {
					RETURN_TOKEN(':')
				} else {
					NEXT();
					RETURN_TOKEN(TK_DOUBLE_COLON);
				}
			case _LC('*'):
				NEXT();
				if (CUR_CHAR == _LC('=')) {
					NEXT();
					RETURN_TOKEN(TK_MULEQ);
				} else RETURN_TOKEN('*');
			case _LC('%'):
				NEXT();
				if (CUR_CHAR == _LC('=')) {
					NEXT();
					RETURN_TOKEN(TK_MODEQ);
				} else RETURN_TOKEN('%');
			case _LC('-'):
				NEXT();
				if (CUR_CHAR == _LC('=')) {
					NEXT();
					RETURN_TOKEN(TK_MINUSEQ);
				} else if  (CUR_CHAR == _LC('-')) {
					NEXT();
					RETURN_TOKEN(TK_MINUSMINUS);
				} else RETURN_TOKEN('-');
			case _LC('+'):
				NEXT();
				if (CUR_CHAR == _LC('=')) {
					NEXT();
					RETURN_TOKEN(TK_PLUSEQ);
				} else if (CUR_CHAR == _LC('+')) {
					NEXT();
					RETURN_TOKEN(TK_PLUSPLUS);
				} else RETURN_TOKEN('+');
			case SQUIRREL_EOB:
				return 0;
			default: {
				if (scisdigit(CUR_CHAR)) {
					SQInteger ret = ReadNumber();
					RETURN_TOKEN(ret);
				} else if (scisalpha(CUR_CHAR) || CUR_CHAR == _LC('_')) {
					SQInteger t = ReadID();
					RETURN_TOKEN(t);
				} else {
					SQInteger c = CUR_CHAR;
					if (sciscntrl((int)c))
						Error(_LC("unexpected character(control)"));
					NEXT();
					RETURN_TOKEN(c);
				}
				RETURN_TOKEN(0);
			}
		}
	}
	return 0;
}

SQInteger LVLexer::GetIDType(const SQChar *s, SQInteger len) {
	SQObjectPtr t;
	if (_keywords->GetStr(s, len, t)) {
		return SQInteger(_integer(t));
	}

	return TK_IDENTIFIER;
}

#ifdef LVUNICODE
#if WCHAR_SIZE == 2
SQInteger LVLexer::AddUTF16(SQUnsignedInteger ch) {
	if (ch >= 0x10000) {
		SQUnsignedInteger code = (ch - 0x10000);
		APPEND_CHAR((SQChar)(0xD800 | (code >> 10)));
		APPEND_CHAR((SQChar)(0xDC00 | (code & 0x3FF)));
		return 2;
	} else {
		APPEND_CHAR((SQChar)ch);
		return 1;
	}
}
#endif
#else
SQInteger LVLexer::AddUTF8(SQUnsignedInteger ch) {
	if (ch < 0x80) {
		APPEND_CHAR((char)ch);
		return 1;
	}
	if (ch < 0x800) {
		APPEND_CHAR((SQChar)((ch >> 6) | 0xC0));
		APPEND_CHAR((SQChar)((ch & 0x3F) | 0x80));
		return 2;
	}
	if (ch < 0x10000) {
		APPEND_CHAR((SQChar)((ch >> 12) | 0xE0));
		APPEND_CHAR((SQChar)(((ch >> 6) & 0x3F) | 0x80));
		APPEND_CHAR((SQChar)((ch & 0x3F) | 0x80));
		return 3;
	}
	if (ch < 0x110000) {
		APPEND_CHAR((SQChar)((ch >> 18) | 0xF0));
		APPEND_CHAR((SQChar)(((ch >> 12) & 0x3F) | 0x80));
		APPEND_CHAR((SQChar)(((ch >> 6) & 0x3F) | 0x80));
		APPEND_CHAR((SQChar)((ch & 0x3F) | 0x80));
		return 4;
	}
	return 0;
}
#endif

SQInteger LVLexer::ProcessStringHexEscape(SQChar *dest, SQInteger maxdigits) {
	NEXT();
	if (!isxdigit(CUR_CHAR)) Error(_LC("hexadecimal number expected"));
	SQInteger n = 0;
	while (isxdigit(CUR_CHAR) && n < maxdigits) {
		dest[n] = CUR_CHAR;
		n++;
		NEXT();
	}
	dest[n] = 0;
	return n;
}

SQInteger LVLexer::ReadString(SQInteger ndelim, bool verbatim) {
	INIT_TEMP_STRING();
	NEXT();
	if (IS_EOB()) return -1;
	for (;;) {
		while (CUR_CHAR != ndelim) {
			SQInteger x = CUR_CHAR;
			switch (x) {
				case SQUIRREL_EOB:
					Error(_LC("unfinished string"));
					return -1;
				case _LC('\n'):
					if (!verbatim) Error(_LC("newline in a constant"));
					APPEND_CHAR(CUR_CHAR);
					NEXT();
					_currentline++;
					break;
				case _LC('\\'):
					if (verbatim) {
						APPEND_CHAR('\\');
						NEXT();
					} else {
						NEXT();
						switch (CUR_CHAR) {
							case _LC('x'):  {
								const SQInteger maxdigits = sizeof(SQChar) * 2;
								SQChar temp[maxdigits + 1];
								ProcessStringHexEscape(temp, maxdigits);
								SQChar *stemp;
								APPEND_CHAR((SQChar)scstrtoul(temp, &stemp, 16));
							}
							break;
							case _LC('U'):
							case _LC('u'):  {
								const SQInteger maxdigits = x == 'u' ? 4 : 8;
								SQChar temp[8 + 1];
								ProcessStringHexEscape(temp, maxdigits);
								SQChar *stemp;
#ifdef LVUNICODE
#if WCHAR_SIZE == 2
								AddUTF16(scstrtoul(temp, &stemp, 16));
#else
								ADD_CHAR((SQChar)scstrtoul(temp, &stemp, 16));
#endif
#else
								AddUTF8(scstrtoul(temp, &stemp, 16));
#endif
							}
							break;
							case _LC('t'):
								APPEND_CHAR(_LC('\t'));
								NEXT();
								break;
							case _LC('a'):
								APPEND_CHAR(_LC('\a'));
								NEXT();
								break;
							case _LC('b'):
								APPEND_CHAR(_LC('\b'));
								NEXT();
								break;
							case _LC('n'):
								APPEND_CHAR(_LC('\n'));
								NEXT();
								break;
							case _LC('r'):
								APPEND_CHAR(_LC('\r'));
								NEXT();
								break;
							case _LC('v'):
								APPEND_CHAR(_LC('\v'));
								NEXT();
								break;
							case _LC('f'):
								APPEND_CHAR(_LC('\f'));
								NEXT();
								break;
							case _LC('0'):
								APPEND_CHAR(_LC('\0'));
								NEXT();
								break;
							case _LC('\\'):
								APPEND_CHAR(_LC('\\'));
								NEXT();
								break;
							case _LC('"'):
								APPEND_CHAR(_LC('"'));
								NEXT();
								break;
							case _LC('\''):
								APPEND_CHAR(_LC('\''));
								NEXT();
								break;
							default:
								Error(_LC("unrecognised escaper char"));
								break;
						}
					}
					break;
				default:
					APPEND_CHAR(CUR_CHAR);
					NEXT();
			}
		}
		NEXT();
		if (verbatim && CUR_CHAR == '"') { //double quotation
			APPEND_CHAR(CUR_CHAR);
			NEXT();
		} else {
			break;
		}
	}
	TERMINATE_BUFFER();
	SQInteger len = _longstr.size() - 1;
	if (ndelim == _LC('\'')) {
		if (len == 0) Error(_LC("empty constant"));
		if (len > 1) Error(_LC("constant too long"));
		_nvalue = _longstr[0];
		return TK_INTEGER;
	}
	_svalue = &_longstr[0];
	return TK_STRING_LITERAL;
}

void LexHexadecimal(const SQChar *s, SQUnsignedInteger *res) {
	*res = 0;
	while (*s != 0) {
		if (scisdigit(*s)) *res = (*res) * 16 + ((*s++) - '0');
		else if (scisxdigit(*s)) *res = (*res) * 16 + (toupper(*s++) - 'A' + 10);
		else {
			assert(0);
		}
	}
}

void LexInteger(const SQChar *s, SQUnsignedInteger *res) {
	*res = 0;
	while (*s != 0) {
		*res = (*res) * 10 + ((*s++) - '0');
	}
}

SQInteger scisodigit(SQInteger c) {
	return c >= _LC('0') && c <= _LC('7');
}

void LexOctal(const SQChar *s, SQUnsignedInteger *res) {
	*res = 0;
	while (*s != 0) {
		if (scisodigit(*s)) *res = (*res) * 8 + ((*s++) - '0');
		else {
			assert(0);
		}
	}
}

SQInteger isexponent(SQInteger c) {
	return c == 'e' || c == 'E';
}

#define MAX_HEX_DIGITS (sizeof(SQInteger)*2)
SQInteger LVLexer::ReadNumber() {
#define TINT 1
#define TFLOAT 2
#define THEX 3
#define TSCIENTIFIC 4
#define TOCTAL 5
	SQInteger type = TINT, firstchar = CUR_CHAR;
	SQChar *sTemp;
	INIT_TEMP_STRING();
	NEXT();
	if (firstchar == _LC('0') && (toupper(CUR_CHAR) == _LC('X') || scisodigit(CUR_CHAR)) ) {
		if (scisodigit(CUR_CHAR)) {
			type = TOCTAL;
			while (scisodigit(CUR_CHAR)) {
				APPEND_CHAR(CUR_CHAR);
				NEXT();
			}
			if (scisdigit(CUR_CHAR)) Error(_LC("invalid octal number"));
		} else {
			NEXT();
			type = THEX;
			while (isxdigit(CUR_CHAR)) {
				APPEND_CHAR(CUR_CHAR);
				NEXT();
			}
			if (_longstr.size() > MAX_HEX_DIGITS) Error(_LC("too many digits for an Hex number"));
		}
	} else {
		APPEND_CHAR((int)firstchar);
		while (CUR_CHAR == _LC('.') || scisdigit(CUR_CHAR) || isexponent(CUR_CHAR)) {
			if (CUR_CHAR == _LC('.') || isexponent(CUR_CHAR)) type = TFLOAT;
			if (isexponent(CUR_CHAR)) {
				if (type != TFLOAT)
					Error(_LC("invalid numeric format"));

				type = TSCIENTIFIC;
				APPEND_CHAR(CUR_CHAR);
				NEXT();
				if (CUR_CHAR == '+' || CUR_CHAR == '-') {
					APPEND_CHAR(CUR_CHAR);
					NEXT();
				}
				if (!scisdigit(CUR_CHAR)) Error(_LC("exponent expected"));
			}

			APPEND_CHAR(CUR_CHAR);
			NEXT();
		}
	}
	TERMINATE_BUFFER();
	switch (type) {
		case TSCIENTIFIC:
		case TFLOAT:
			_fvalue = (SQFloat)scstrtod(&_longstr[0], &sTemp);
			return TK_FLOAT;
		case TINT:
			LexInteger(&_longstr[0], (SQUnsignedInteger *)&_nvalue);
			return TK_INTEGER;
		case THEX:
			LexHexadecimal(&_longstr[0], (SQUnsignedInteger *)&_nvalue);
			return TK_INTEGER;
		case TOCTAL:
			LexOctal(&_longstr[0], (SQUnsignedInteger *)&_nvalue);
			return TK_INTEGER;
	}
	return 0;
}

SQInteger LVLexer::ReadID() {
	SQInteger res;

	INIT_TEMP_STRING();

	do {
		APPEND_CHAR(CUR_CHAR);
		NEXT();
	} while (scisalnum(CUR_CHAR) || CUR_CHAR == _LC('_'));

	TERMINATE_BUFFER();

	res = GetIDType(&_longstr[0], _longstr.size() - 1);
	if (res == TK_IDENTIFIER || res == TK_CONSTRUCTOR) {
		_svalue = &_longstr[0];
	}

	return res;
}
