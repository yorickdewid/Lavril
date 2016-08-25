#include <lavril.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <sqstdstring.h>

#ifdef _DEBUG
#include <stdio.h>

static const LVChar *g_nnames[] = {
	_LC("NONE"),
	_LC("OP_GREEDY"),
	_LC("OP_OR"),
	_LC("OP_EXPR"),
	_LC("OP_NOCAPEXPR"),
	_LC("OP_DOT"),
	_LC("OP_CLASS"),
	_LC("OP_CCLASS"),
	_LC("OP_NCLASS"),
	_LC("OP_RANGE"),
	_LC("OP_CHAR"),
	_LC("OP_EOL"),
	_LC("OP_BOL"),
	_LC("OP_WB"),
	_LC("OP_MB")
};

#endif

#define OP_GREEDY       (MAX_CHAR+1) // * + ? {n}
#define OP_OR           (MAX_CHAR+2)
#define OP_EXPR         (MAX_CHAR+3) //parentesis ()
#define OP_NOCAPEXPR    (MAX_CHAR+4) //parentesis (?:)
#define OP_DOT          (MAX_CHAR+5)
#define OP_CLASS        (MAX_CHAR+6)
#define OP_CCLASS       (MAX_CHAR+7)
#define OP_NCLASS       (MAX_CHAR+8) //negates class the [^
#define OP_RANGE        (MAX_CHAR+9)
#define OP_CHAR         (MAX_CHAR+10)
#define OP_EOL          (MAX_CHAR+11)
#define OP_BOL          (MAX_CHAR+12)
#define OP_WB           (MAX_CHAR+13)
#define OP_MB           (MAX_CHAR+14) //match balanced

#define SQREX_SYMBOL_ANY_CHAR ('.')
#define SQREX_SYMBOL_GREEDY_ONE_OR_MORE ('+')
#define SQREX_SYMBOL_GREEDY_ZERO_OR_MORE ('*')
#define SQREX_SYMBOL_GREEDY_ZERO_OR_ONE ('?')
#define SQREX_SYMBOL_BRANCH ('|')
#define SQREX_SYMBOL_END_OF_STRING ('$')
#define SQREX_SYMBOL_BEGINNING_OF_STRING ('^')
#define SQREX_SYMBOL_ESCAPE_CHAR ('\\')

typedef int SQRexNodeType;

typedef struct tagSQRexNode {
	SQRexNodeType type;
	LVInteger left;
	LVInteger right;
	LVInteger next;
} SQRexNode;

struct SQRex {
	const LVChar *_eol;
	const LVChar *_bol;
	const LVChar *_p;
	LVInteger _first;
	LVInteger _op;
	SQRexNode *_nodes;
	LVInteger _nallocated;
	LVInteger _nsize;
	LVInteger _nsubexpr;
	SQRexMatch *_matches;
	LVInteger _currsubexp;
	void *_jmpbuf;
	const LVChar **_error;
};

static LVInteger sqstd_rex_list(SQRex *exp);

static LVInteger sqstd_rex_newnode(SQRex *exp, SQRexNodeType type) {
	SQRexNode n;
	n.type = type;
	n.next = n.right = n.left = -1;
	if (type == OP_EXPR)
		n.right = exp->_nsubexpr++;
	if (exp->_nallocated < (exp->_nsize + 1)) {
		LVInteger oldsize = exp->_nallocated;
		exp->_nallocated *= 2;
		exp->_nodes = (SQRexNode *)sq_realloc(exp->_nodes, oldsize * sizeof(SQRexNode) , exp->_nallocated * sizeof(SQRexNode));
	}
	exp->_nodes[exp->_nsize++] = n;
	LVInteger newid = exp->_nsize - 1;
	return (LVInteger)newid;
}

static void sqstd_rex_error(SQRex *exp, const LVChar *error) {
	if (exp->_error) *exp->_error = error;
	longjmp(*((jmp_buf *)exp->_jmpbuf), -1);
}

static void sqstd_rex_expect(SQRex *exp, LVInteger n) {
	if ((*exp->_p) != n)
		sqstd_rex_error(exp, _LC("expected paren"));
	exp->_p++;
}

static LVChar sqstd_rex_escapechar(SQRex *exp) {
	if (*exp->_p == SQREX_SYMBOL_ESCAPE_CHAR) {
		exp->_p++;
		switch (*exp->_p) {
			case 'v':
				exp->_p++;
				return '\v';
			case 'n':
				exp->_p++;
				return '\n';
			case 't':
				exp->_p++;
				return '\t';
			case 'r':
				exp->_p++;
				return '\r';
			case 'f':
				exp->_p++;
				return '\f';
			default:
				return (*exp->_p++);
		}
	} else if (!scisprint(*exp->_p)) sqstd_rex_error(exp, _LC("letter expected"));
	return (*exp->_p++);
}

static LVInteger sqstd_rex_charclass(SQRex *exp, LVInteger classid) {
	LVInteger n = sqstd_rex_newnode(exp, OP_CCLASS);
	exp->_nodes[n].left = classid;
	return n;
}

static LVInteger sqstd_rex_charnode(SQRex *exp, LVBool isclass) {
	LVChar t;
	if (*exp->_p == SQREX_SYMBOL_ESCAPE_CHAR) {
		exp->_p++;
		switch (*exp->_p) {
			case 'n':
				exp->_p++;
				return sqstd_rex_newnode(exp, '\n');
			case 't':
				exp->_p++;
				return sqstd_rex_newnode(exp, '\t');
			case 'r':
				exp->_p++;
				return sqstd_rex_newnode(exp, '\r');
			case 'f':
				exp->_p++;
				return sqstd_rex_newnode(exp, '\f');
			case 'v':
				exp->_p++;
				return sqstd_rex_newnode(exp, '\v');
			case 'a':
			case 'A':
			case 'w':
			case 'W':
			case 's':
			case 'S':
			case 'd':
			case 'D':
			case 'x':
			case 'X':
			case 'c':
			case 'C':
			case 'p':
			case 'P':
			case 'l':
			case 'u': {
				t = *exp->_p;
				exp->_p++;
				return sqstd_rex_charclass(exp, t);
			}
			case 'm': {
				LVChar cb, ce; //cb = character begin match ce = character end match
				cb = *++exp->_p; //skip 'm'
				ce = *++exp->_p;
				exp->_p++; //points to the next char to be parsed
				if ((!cb) || (!ce)) sqstd_rex_error(exp, _LC("balanced chars expected"));
				if ( cb == ce ) sqstd_rex_error(exp, _LC("open/close char can't be the same"));
				LVInteger node =  sqstd_rex_newnode(exp, OP_MB);
				exp->_nodes[node].left = cb;
				exp->_nodes[node].right = ce;
				return node;
			}
			case 'b':
			case 'B':
				if (!isclass) {
					LVInteger node = sqstd_rex_newnode(exp, OP_WB);
					exp->_nodes[node].left = *exp->_p;
					exp->_p++;
					return node;
				} //else default
			default:
				t = *exp->_p;
				exp->_p++;
				return sqstd_rex_newnode(exp, t);
		}
	} else if (!scisprint(*exp->_p)) {

		sqstd_rex_error(exp, _LC("letter expected"));
	}
	t = *exp->_p;
	exp->_p++;
	return sqstd_rex_newnode(exp, t);
}
static LVInteger sqstd_rex_class(SQRex *exp) {
	LVInteger ret = -1;
	LVInteger first = -1, chain;
	if (*exp->_p == SQREX_SYMBOL_BEGINNING_OF_STRING) {
		ret = sqstd_rex_newnode(exp, OP_NCLASS);
		exp->_p++;
	} else ret = sqstd_rex_newnode(exp, OP_CLASS);

	if (*exp->_p == ']') sqstd_rex_error(exp, _LC("empty class"));
	chain = ret;
	while (*exp->_p != ']' && exp->_p != exp->_eol) {
		if (*exp->_p == '-' && first != -1) {
			LVInteger r;
			if (*exp->_p++ == ']') sqstd_rex_error(exp, _LC("unfinished range"));
			r = sqstd_rex_newnode(exp, OP_RANGE);
			if (exp->_nodes[first].type > *exp->_p) sqstd_rex_error(exp, _LC("invalid range"));
			if (exp->_nodes[first].type == OP_CCLASS) sqstd_rex_error(exp, _LC("cannot use character classes in ranges"));
			exp->_nodes[r].left = exp->_nodes[first].type;
			LVInteger t = sqstd_rex_escapechar(exp);
			exp->_nodes[r].right = t;
			exp->_nodes[chain].next = r;
			chain = r;
			first = -1;
		} else {
			if (first != -1) {
				LVInteger c = first;
				exp->_nodes[chain].next = c;
				chain = c;
				first = sqstd_rex_charnode(exp, LVTrue);
			} else {
				first = sqstd_rex_charnode(exp, LVTrue);
			}
		}
	}
	if (first != -1) {
		LVInteger c = first;
		exp->_nodes[chain].next = c;
	}
	/* hack? */
	exp->_nodes[ret].left = exp->_nodes[ret].next;
	exp->_nodes[ret].next = -1;
	return ret;
}

static LVInteger sqstd_rex_parsenumber(SQRex *exp) {
	LVInteger ret = *exp->_p - '0';
	LVInteger positions = 10;
	exp->_p++;
	while (isdigit(*exp->_p)) {
		ret = ret * 10 + (*exp->_p++ -'0');
		if (positions == 1000000000)
			sqstd_rex_error(exp, _LC("overflow in numeric constant"));
		positions *= 10;
	};
	return ret;
}

static LVInteger sqstd_rex_element(SQRex *exp) {
	LVInteger ret = -1;
	switch (*exp->_p) {
		case '(': {
			LVInteger expr;
			exp->_p++;
			if (*exp->_p == '?') {
				exp->_p++;
				sqstd_rex_expect(exp, ':');
				expr = sqstd_rex_newnode(exp, OP_NOCAPEXPR);
			} else {
				expr = sqstd_rex_newnode(exp, OP_EXPR);
			}

			LVInteger newn = sqstd_rex_list(exp);
			exp->_nodes[expr].left = newn;
			ret = expr;
			sqstd_rex_expect(exp, ')');
		}
		break;
		case '[':
			exp->_p++;
			ret = sqstd_rex_class(exp);
			sqstd_rex_expect(exp, ']');
			break;
		case SQREX_SYMBOL_END_OF_STRING:
			exp->_p++;
			ret = sqstd_rex_newnode(exp, OP_EOL);
			break;
		case SQREX_SYMBOL_ANY_CHAR:
			exp->_p++;
			ret = sqstd_rex_newnode(exp, OP_DOT);
			break;
		default:
			ret = sqstd_rex_charnode(exp, LVFalse);
			break;
	}

	LVBool isgreedy = LVFalse;
	unsigned short p0 = 0, p1 = 0;
	switch (*exp->_p) {
		case SQREX_SYMBOL_GREEDY_ZERO_OR_MORE:
			p0 = 0;
			p1 = 0xFFFF;
			exp->_p++;
			isgreedy = LVTrue;
			break;
		case SQREX_SYMBOL_GREEDY_ONE_OR_MORE:
			p0 = 1;
			p1 = 0xFFFF;
			exp->_p++;
			isgreedy = LVTrue;
			break;
		case SQREX_SYMBOL_GREEDY_ZERO_OR_ONE:
			p0 = 0;
			p1 = 1;
			exp->_p++;
			isgreedy = LVTrue;
			break;
		case '{':
			exp->_p++;
			if (!isdigit(*exp->_p))
				sqstd_rex_error(exp, _LC("number expected"));
			p0 = (unsigned short)sqstd_rex_parsenumber(exp);
			/*******************************/
			switch (*exp->_p) {
				case '}':
					p1 = p0;
					exp->_p++;
					break;
				case ',':
					exp->_p++;
					p1 = 0xFFFF;
					if (isdigit(*exp->_p)) {
						p1 = (unsigned short)sqstd_rex_parsenumber(exp);
					}
					sqstd_rex_expect(exp, '}');
					break;
				default:
					sqstd_rex_error(exp, _LC(", or } expected"));
			}
			/*******************************/
			isgreedy = LVTrue;
			break;

	}
	if (isgreedy) {
		LVInteger nnode = sqstd_rex_newnode(exp, OP_GREEDY);
		exp->_nodes[nnode].left = ret;
		exp->_nodes[nnode].right = ((p0) << 16) | p1;
		ret = nnode;
	}

	if ((*exp->_p != SQREX_SYMBOL_BRANCH) && (*exp->_p != ')') && (*exp->_p != SQREX_SYMBOL_GREEDY_ZERO_OR_MORE) && (*exp->_p != SQREX_SYMBOL_GREEDY_ONE_OR_MORE) && (*exp->_p != '\0')) {
		LVInteger nnode = sqstd_rex_element(exp);
		exp->_nodes[ret].next = nnode;
	}

	return ret;
}

static LVInteger sqstd_rex_list(SQRex *exp) {
	LVInteger ret = -1, e;
	if (*exp->_p == SQREX_SYMBOL_BEGINNING_OF_STRING) {
		exp->_p++;
		ret = sqstd_rex_newnode(exp, OP_BOL);
	}
	e = sqstd_rex_element(exp);
	if (ret != -1) {
		exp->_nodes[ret].next = e;
	} else ret = e;

	if (*exp->_p == SQREX_SYMBOL_BRANCH) {
		LVInteger temp, tright;
		exp->_p++;
		temp = sqstd_rex_newnode(exp, OP_OR);
		exp->_nodes[temp].left = ret;
		tright = sqstd_rex_list(exp);
		exp->_nodes[temp].right = tright;
		ret = temp;
	}
	return ret;
}

static LVBool sqstd_rex_matchcclass(LVInteger cclass, LVChar c) {
	switch (cclass) {
		case 'a':
			return isalpha(c) ? LVTrue : LVFalse;
		case 'A':
			return !isalpha(c) ? LVTrue : LVFalse;
		case 'w':
			return (isalnum(c) || c == '_') ? LVTrue : LVFalse;
		case 'W':
			return (!isalnum(c) && c != '_') ? LVTrue : LVFalse;
		case 's':
			return isspace(c) ? LVTrue : LVFalse;
		case 'S':
			return !isspace(c) ? LVTrue : LVFalse;
		case 'd':
			return isdigit(c) ? LVTrue : LVFalse;
		case 'D':
			return !isdigit(c) ? LVTrue : LVFalse;
		case 'x':
			return isxdigit(c) ? LVTrue : LVFalse;
		case 'X':
			return !isxdigit(c) ? LVTrue : LVFalse;
		case 'c':
			return iscntrl(c) ? LVTrue : LVFalse;
		case 'C':
			return !iscntrl(c) ? LVTrue : LVFalse;
		case 'p':
			return ispunct(c) ? LVTrue : LVFalse;
		case 'P':
			return !ispunct(c) ? LVTrue : LVFalse;
		case 'l':
			return islower(c) ? LVTrue : LVFalse;
		case 'u':
			return isupper(c) ? LVTrue : LVFalse;
	}
	return LVFalse; /*cannot happen*/
}

static LVBool sqstd_rex_matchclass(SQRex *exp, SQRexNode *node, LVChar c) {
	do {
		switch (node->type) {
			case OP_RANGE:
				if (c >= node->left && c <= node->right) return LVTrue;
				break;
			case OP_CCLASS:
				if (sqstd_rex_matchcclass(node->left, c)) return LVTrue;
				break;
			default:
				if (c == node->type)return LVTrue;
		}
	} while ((node->next != -1) && (node = &exp->_nodes[node->next]));
	return LVFalse;
}

static const LVChar *sqstd_rex_matchnode(SQRex *exp, SQRexNode *node, const LVChar *str, SQRexNode *next) {

	SQRexNodeType type = node->type;
	switch (type) {
		case OP_GREEDY: {
			//SQRexNode *greedystop = (node->next != -1) ? &exp->_nodes[node->next] : NULL;
			SQRexNode *greedystop = NULL;
			LVInteger p0 = (node->right >> 16) & 0x0000FFFF, p1 = node->right & 0x0000FFFF, nmaches = 0;
			const LVChar *s = str, *good = str;

			if (node->next != -1) {
				greedystop = &exp->_nodes[node->next];
			} else {
				greedystop = next;
			}

			while ((nmaches == 0xFFFF || nmaches < p1)) {

				const LVChar *stop;
				if (!(s = sqstd_rex_matchnode(exp, &exp->_nodes[node->left], s, greedystop)))
					break;
				nmaches++;
				good = s;
				if (greedystop) {
					//checks that 0 matches satisfy the expression(if so skips)
					//if not would always stop(for instance if is a '?')
					if (greedystop->type != OP_GREEDY ||
					        (greedystop->type == OP_GREEDY && ((greedystop->right >> 16) & 0x0000FFFF) != 0)) {
						SQRexNode *gnext = NULL;
						if (greedystop->next != -1) {
							gnext = &exp->_nodes[greedystop->next];
						} else if (next && next->next != -1) {
							gnext = &exp->_nodes[next->next];
						}
						stop = sqstd_rex_matchnode(exp, greedystop, s, gnext);
						if (stop) {
							//if satisfied stop it
							if (p0 == p1 && p0 == nmaches)
								break;
							else if (nmaches >= p0 && p1 == 0xFFFF)
								break;
							else if (nmaches >= p0 && nmaches <= p1)
								break;
						}
					}
				}

				if (s >= exp->_eol)
					break;
			}
			if (p0 == p1 && p0 == nmaches) return good;
			else if (nmaches >= p0 && p1 == 0xFFFF) return good;
			else if (nmaches >= p0 && nmaches <= p1) return good;
			return NULL;
		}
		case OP_OR: {
			const LVChar *asd = str;
			SQRexNode *temp = &exp->_nodes[node->left];
			while ( (asd = sqstd_rex_matchnode(exp, temp, asd, NULL)) ) {
				if (temp->next != -1)
					temp = &exp->_nodes[temp->next];
				else
					return asd;
			}
			asd = str;
			temp = &exp->_nodes[node->right];
			while ( (asd = sqstd_rex_matchnode(exp, temp, asd, NULL)) ) {
				if (temp->next != -1)
					temp = &exp->_nodes[temp->next];
				else
					return asd;
			}
			return NULL;
			break;
		}
		case OP_EXPR:
		case OP_NOCAPEXPR: {
			SQRexNode *n = &exp->_nodes[node->left];
			const LVChar *cur = str;
			LVInteger capture = -1;
			if (node->type != OP_NOCAPEXPR && node->right == exp->_currsubexp) {
				capture = exp->_currsubexp;
				exp->_matches[capture].begin = cur;
				exp->_currsubexp++;
			}
			LVInteger tempcap = exp->_currsubexp;
			do {
				SQRexNode *subnext = NULL;
				if (n->next != -1) {
					subnext = &exp->_nodes[n->next];
				} else {
					subnext = next;
				}
				if (!(cur = sqstd_rex_matchnode(exp, n, cur, subnext))) {
					if (capture != -1) {
						exp->_matches[capture].begin = 0;
						exp->_matches[capture].len = 0;
					}
					return NULL;
				}
			} while ((n->next != -1) && (n = &exp->_nodes[n->next]));

			exp->_currsubexp = tempcap;
			if (capture != -1)
				exp->_matches[capture].len = cur - exp->_matches[capture].begin;
			return cur;
		}
		case OP_WB:
			if ((str == exp->_bol && !isspace(*str))
			        || (str == exp->_eol && !isspace(*(str - 1)))
			        || (!isspace(*str) && isspace(*(str + 1)))
			        || (isspace(*str) && !isspace(*(str + 1))) ) {
				return (node->left == 'b') ? str : NULL;
			}
			return (node->left == 'b') ? NULL : str;
		case OP_BOL:
			if (str == exp->_bol) return str;
			return NULL;
		case OP_EOL:
			if (str == exp->_eol) return str;
			return NULL;
		case OP_DOT: {
			if (str == exp->_eol) return NULL;
			str++;
		}
		return str;
		case OP_NCLASS:
		case OP_CLASS:
			if (str == exp->_eol) return NULL;
			if (sqstd_rex_matchclass(exp, &exp->_nodes[node->left], *str) ? (type == OP_CLASS ? LVTrue : LVFalse) : (type == OP_NCLASS ? LVTrue : LVFalse)) {
				str++;
				return str;
			}
			return NULL;
		case OP_CCLASS:
			if (str == exp->_eol) return NULL;
			if (sqstd_rex_matchcclass(node->left, *str)) {
				str++;
				return str;
			}
			return NULL;
		case OP_MB: {
			LVInteger cb = node->left; //char that opens a balanced expression
			if (*str != cb) return NULL; // string doesnt start with open char
			LVInteger ce = node->right; //char that closes a balanced expression
			LVInteger cont = 1;
			const LVChar *streol = exp->_eol;
			while (++str < streol) {
				if (*str == ce) {
					if (--cont == 0) {
						return ++str;
					}
				} else if (*str == cb) cont++;
			}
		}
		return NULL; // string ends out of balance
		default: /* char */
			if (str == exp->_eol) return NULL;
			if (*str != node->type) return NULL;
			str++;
			return str;
	}
	return NULL;
}

/* public api */
SQRex *sqstd_rex_compile(const LVChar *pattern, const LVChar **error) {
	SQRex *volatile exp = (SQRex *)lv_malloc(sizeof(SQRex));  // "volatile" is needed for setjmp()
	exp->_eol = exp->_bol = NULL;
	exp->_p = pattern;
	exp->_nallocated = (LVInteger)scstrlen(pattern) * sizeof(LVChar);
	exp->_nodes = (SQRexNode *)lv_malloc(exp->_nallocated * sizeof(SQRexNode));
	exp->_nsize = 0;
	exp->_matches = 0;
	exp->_nsubexpr = 0;
	exp->_first = sqstd_rex_newnode(exp, OP_EXPR);
	exp->_error = error;
	exp->_jmpbuf = lv_malloc(sizeof(jmp_buf));
	if (setjmp(*((jmp_buf *)exp->_jmpbuf)) == 0) {
		LVInteger res = sqstd_rex_list(exp);
		exp->_nodes[exp->_first].left = res;
		if (*exp->_p != '\0')
			sqstd_rex_error(exp, _LC("unexpected character"));
#ifdef _DEBUG
		{
			LVInteger nsize, i;
			SQRexNode *t;
			nsize = exp->_nsize;
			t = &exp->_nodes[0];
			scprintf(_LC("\n"));
			for (i = 0; i < nsize; i++) {
				if (exp->_nodes[i].type > MAX_CHAR)
					scprintf(_LC("[%02d] %10s "), i, g_nnames[exp->_nodes[i].type - MAX_CHAR]);
				else
					scprintf(_LC("[%02d] %10c "), i, exp->_nodes[i].type);
				scprintf(_LC("left %02d right %02d next %02d\n"), (SQInt32)exp->_nodes[i].left, (SQInt32)exp->_nodes[i].right, (SQInt32)exp->_nodes[i].next);
			}
			scprintf(_LC("\n"));
		}
#endif
		exp->_matches = (SQRexMatch *) lv_malloc(exp->_nsubexpr * sizeof(SQRexMatch));
		memset(exp->_matches, 0, exp->_nsubexpr * sizeof(SQRexMatch));
	} else {
		sqstd_rex_free(exp);
		return NULL;
	}
	return exp;
}

void sqstd_rex_free(SQRex *exp) {
	if (exp) {
		if (exp->_nodes)
			lv_free(exp->_nodes, exp->_nallocated * sizeof(SQRexNode));
		if (exp->_jmpbuf)
			lv_free(exp->_jmpbuf, sizeof(jmp_buf));
		if (exp->_matches)
			lv_free(exp->_matches, exp->_nsubexpr * sizeof(SQRexMatch));
		lv_free(exp, sizeof(SQRex));
	}
}

LVBool sqstd_rex_match(SQRex *exp, const LVChar *text) {
	const LVChar *res = NULL;
	exp->_bol = text;
	exp->_eol = text + scstrlen(text);
	exp->_currsubexp = 0;
	res = sqstd_rex_matchnode(exp, exp->_nodes, text, NULL);
	if (res == NULL || res != exp->_eol)
		return LVFalse;
	return LVTrue;
}

LVBool sqstd_rex_searchrange(SQRex *exp, const LVChar *text_begin, const LVChar *text_end, const LVChar **out_begin, const LVChar **out_end) {
	const LVChar *cur = NULL;
	LVInteger node = exp->_first;
	if (text_begin >= text_end) return LVFalse;
	exp->_bol = text_begin;
	exp->_eol = text_end;
	do {
		cur = text_begin;
		while (node != -1) {
			exp->_currsubexp = 0;
			cur = sqstd_rex_matchnode(exp, &exp->_nodes[node], cur, NULL);
			if (!cur)
				break;
			node = exp->_nodes[node].next;
		}
		text_begin++;
	} while (cur == NULL && text_begin != text_end);

	if (cur == NULL)
		return LVFalse;

	--text_begin;

	if (out_begin) *out_begin = text_begin;
	if (out_end) *out_end = cur;
	return LVTrue;
}

LVBool sqstd_rex_search(SQRex *exp, const LVChar *text, const LVChar **out_begin, const LVChar **out_end) {
	return sqstd_rex_searchrange(exp, text, text + scstrlen(text), out_begin, out_end);
}

LVInteger sqstd_rex_getsubexpcount(SQRex *exp) {
	return exp->_nsubexpr;
}

LVBool sqstd_rex_getsubexp(SQRex *exp, LVInteger n, SQRexMatch *subexp) {
	if ( n < 0 || n >= exp->_nsubexpr) return LVFalse;
	*subexp = exp->_matches[n];
	return LVTrue;
}
