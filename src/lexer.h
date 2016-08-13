#ifndef _LEXER_H_
#define _LEXER_H_

#ifdef LVUNICODE
typedef LVChar LexChar;
#else
typedef unsigned char LexChar;
#endif

struct LVLexer {
	LVLexer();
	~LVLexer();
	void Init(SQSharedState *ss, SQLEXREADFUNC rg, LVUserPointer up, CompilerErrorFunc efunc, void *ed);
	void Error(const LVChar *err);
	LVInteger Lex();
	const LVChar *Tok2Str(LVInteger tok);

  private:
	LVInteger GetIDType(const LVChar *s, LVInteger len);
	LVInteger ReadString(LVInteger ndelim, bool verbatim);
	LVInteger ReadNumber();
	void LexBlockComment();
	void LexLineComment();
	LVInteger ReadID();
	void Next();
#ifdef LVUNICODE
#if WCHAR_SIZE == 2
	LVInteger AddUTF16(LVUnsignedInteger ch);
#endif
#else
	LVInteger AddUTF8(LVUnsignedInteger ch);
#endif
	LVInteger ProcessStringHexEscape(LVChar *dest, LVInteger maxdigits);
	LVInteger _curtoken;
	SQTable *_keywords;
	LVBool _reached_eof;

  public:
	LVInteger _prevtoken;
	LVInteger _currentline;
	LVInteger _lasttokenline;
	LVInteger _currentcolumn;
	const LVChar *_svalue;
	LVInteger _nvalue;
	LVFloat _fvalue;
	SQLEXREADFUNC _readf;
	LVUserPointer _up;
	LexChar _currdata;
	SQSharedState *_sharedstate;
	sqvector<LVChar> _longstr;
	CompilerErrorFunc _errfunc;
	void *_errtarget;
};

#endif // _LEXER_H_
