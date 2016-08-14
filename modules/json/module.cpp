#include <lavril.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/**
 * JSON type identifier. Basic types are:
 * 	o Object
 * 	o Array
 * 	o String
 * 	o Other primitive: number, boolean (true/false) or null
 */
typedef enum {
	JSMN_UNDEFINED = 0,
	JSMN_OBJECT = 1,
	JSMN_ARRAY = 2,
	JSMN_STRING = 3,
	JSMN_PRIMITIVE = 4
} jsmntype_t;

enum jsmnerr {
	/* Not enough tokens were provided */
	JSMN_ERROR_NOMEM = -1,
	/* Invalid character inside JSON string */
	JSMN_ERROR_INVAL = -2,
	/* The string is not a full JSON packet, more bytes expected */
	JSMN_ERROR_PART = -3
};

/**
 * JSON token description.
 * @param		type	type (object, array, string etc.)
 * @param		start	start position in JSON data string
 * @param		end		end position in JSON data string
 */
typedef struct {
	jsmntype_t type;
	int start;
	int end;
	int size;
#ifdef JSMN_PARENT_LINKS
	int parent;
#endif
} jsmntok_t;

/**
 * JSON parser. Contains an array of token blocks available. Also stores
 * the string being parsed now and current position in that string
 */
typedef struct {
	unsigned int pos; /* offset in the JSON string */
	unsigned int toknext; /* next token to allocate */
	int toksuper; /* superior token node, e.g parent object or array */
} jsmn_parser;

/**
 * Create JSON parser over an array of tokens
 */
void jsmn_init(jsmn_parser *parser);

/**
 * Run JSON parser. It parses a JSON data string into and array of tokens, each describing
 * a single JSON object.
 */
int jsmn_parse(jsmn_parser *parser, const char *js, size_t len,
               jsmntok_t *tokens, unsigned int num_tokens);


/**
 * Allocates a fresh unused token from the token pull.
 */
static jsmntok_t *jsmn_alloc_token(jsmn_parser *parser,
                                   jsmntok_t *tokens, size_t num_tokens) {
	jsmntok_t *tok;
	if (parser->toknext >= num_tokens) {
		return NULL;
	}
	tok = &tokens[parser->toknext++];
	tok->start = tok->end = -1;
	tok->size = 0;
#ifdef JSMN_PARENT_LINKS
	tok->parent = -1;
#endif
	return tok;
}

/**
 * Fills token type and boundaries.
 */
static void jsmn_fill_token(jsmntok_t *token, jsmntype_t type,
                            int start, int end) {
	token->type = type;
	token->start = start;
	token->end = end;
	token->size = 0;
}

/**
 * Fills next available token with JSON primitive.
 */
static int jsmn_parse_primitive(jsmn_parser *parser, const char *js,
                                size_t len, jsmntok_t *tokens, size_t num_tokens) {
	jsmntok_t *token;
	int start;

	start = parser->pos;

	for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) {
		switch (js[parser->pos]) {
#ifndef JSMN_STRICT
			/* In strict mode primitive must be followed by "," or "}" or "]" */
			case ':':
#endif
			case '\t' :
			case '\r' :
			case '\n' :
			case ' ' :
			case ','  :
			case ']'  :
			case '}' :
				goto found;
		}
		if (js[parser->pos] < 32 || js[parser->pos] >= 127) {
			parser->pos = start;
			return JSMN_ERROR_INVAL;
		}
	}
#ifdef JSMN_STRICT
	/* In strict mode primitive must be followed by a comma/object/array */
	parser->pos = start;
	return JSMN_ERROR_PART;
#endif

found:
	if (tokens == NULL) {
		parser->pos--;
		return 0;
	}
	token = jsmn_alloc_token(parser, tokens, num_tokens);
	if (token == NULL) {
		parser->pos = start;
		return JSMN_ERROR_NOMEM;
	}
	jsmn_fill_token(token, JSMN_PRIMITIVE, start, parser->pos);
#ifdef JSMN_PARENT_LINKS
	token->parent = parser->toksuper;
#endif
	parser->pos--;
	return 0;
}

/**
 * Fills next token with JSON string.
 */
static int jsmn_parse_string(jsmn_parser *parser, const char *js,
                             size_t len, jsmntok_t *tokens, size_t num_tokens) {
	jsmntok_t *token;

	int start = parser->pos;

	parser->pos++;

	/* Skip starting quote */
	for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) {
		char c = js[parser->pos];

		/* Quote: end of string */
		if (c == '\"') {
			if (tokens == NULL) {
				return 0;
			}
			token = jsmn_alloc_token(parser, tokens, num_tokens);
			if (token == NULL) {
				parser->pos = start;
				return JSMN_ERROR_NOMEM;
			}
			jsmn_fill_token(token, JSMN_STRING, start + 1, parser->pos);
#ifdef JSMN_PARENT_LINKS
			token->parent = parser->toksuper;
#endif
			return 0;
		}

		/* Backslash: Quoted symbol expected */
		if (c == '\\' && parser->pos + 1 < len) {
			int i;
			parser->pos++;
			switch (js[parser->pos]) {
				/* Allowed escaped symbols */
				case '\"':
				case '/' :
				case '\\' :
				case 'b' :
				case 'f' :
				case 'r' :
				case 'n'  :
				case 't' :
					break;
				/* Allows escaped symbol \uXXXX */
				case 'u':
					parser->pos++;
					for (i = 0; i < 4 && parser->pos < len && js[parser->pos] != '\0'; i++) {
						/* If it isn't a hex character we have an error */
						if (!((js[parser->pos] >= 48 && js[parser->pos] <= 57) || /* 0-9 */
						        (js[parser->pos] >= 65 && js[parser->pos] <= 70) || /* A-F */
						        (js[parser->pos] >= 97 && js[parser->pos] <= 102))) { /* a-f */
							parser->pos = start;
							return JSMN_ERROR_INVAL;
						}
						parser->pos++;
					}
					parser->pos--;
					break;
				/* Unexpected symbol */
				default:
					parser->pos = start;
					return JSMN_ERROR_INVAL;
			}
		}
	}
	parser->pos = start;
	return JSMN_ERROR_PART;
}

/**
 * Parse JSON string and fill tokens.
 */
int jsmn_parse(jsmn_parser *parser, const char *js, size_t len,
               jsmntok_t *tokens, unsigned int num_tokens) {
	int r;
	int i;
	jsmntok_t *token;
	int count = parser->toknext;

	for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) {
		char c;
		jsmntype_t type;

		c = js[parser->pos];
		switch (c) {
			case '{':
			case '[':
				count++;
				if (tokens == NULL) {
					break;
				}
				token = jsmn_alloc_token(parser, tokens, num_tokens);
				if (token == NULL)
					return JSMN_ERROR_NOMEM;
				if (parser->toksuper != -1) {
					tokens[parser->toksuper].size++;
#ifdef JSMN_PARENT_LINKS
					token->parent = parser->toksuper;
#endif
				}
				token->type = (c == '{' ? JSMN_OBJECT : JSMN_ARRAY);
				token->start = parser->pos;
				parser->toksuper = parser->toknext - 1;
				break;
			case '}':
			case ']':
				if (tokens == NULL)
					break;
				type = (c == '}' ? JSMN_OBJECT : JSMN_ARRAY);
#ifdef JSMN_PARENT_LINKS
				if (parser->toknext < 1) {
					return JSMN_ERROR_INVAL;
				}
				token = &tokens[parser->toknext - 1];
				for (;;) {
					if (token->start != -1 && token->end == -1) {
						if (token->type != type) {
							return JSMN_ERROR_INVAL;
						}
						token->end = parser->pos + 1;
						parser->toksuper = token->parent;
						break;
					}
					if (token->parent == -1) {
						break;
					}
					token = &tokens[token->parent];
				}
#else
				for (i = parser->toknext - 1; i >= 0; i--) {
					token = &tokens[i];
					if (token->start != -1 && token->end == -1) {
						if (token->type != type) {
							return JSMN_ERROR_INVAL;
						}
						parser->toksuper = -1;
						token->end = parser->pos + 1;
						break;
					}
				}
				/* Error if unmatched closing bracket */
				if (i == -1)
					return JSMN_ERROR_INVAL;
				for (; i >= 0; i--) {
					token = &tokens[i];
					if (token->start != -1 && token->end == -1) {
						parser->toksuper = i;
						break;
					}
				}
#endif
				break;
			case '\"':
				r = jsmn_parse_string(parser, js, len, tokens, num_tokens);
				if (r < 0) return r;
				count++;
				if (parser->toksuper != -1 && tokens != NULL)
					tokens[parser->toksuper].size++;
				break;
			case '\t' :
			case '\r' :
			case '\n' :
			case ' ':
				break;
			case ':':
				parser->toksuper = parser->toknext - 1;
				break;
			case ',':
				if (tokens != NULL && parser->toksuper != -1 &&
				        tokens[parser->toksuper].type != JSMN_ARRAY &&
				        tokens[parser->toksuper].type != JSMN_OBJECT) {
#ifdef JSMN_PARENT_LINKS
					parser->toksuper = tokens[parser->toksuper].parent;
#else
					for (i = parser->toknext - 1; i >= 0; i--) {
						if (tokens[i].type == JSMN_ARRAY || tokens[i].type == JSMN_OBJECT) {
							if (tokens[i].start != -1 && tokens[i].end == -1) {
								parser->toksuper = i;
								break;
							}
						}
					}
#endif
				}
				break;
#ifdef JSMN_STRICT
			/* In strict mode primitives are: numbers and booleans */
			case '-':
			case '0':
			case '1' :
			case '2':
			case '3' :
			case '4':
			case '5':
			case '6':
			case '7' :
			case '8':
			case '9':
			case 't':
			case 'f':
			case 'n' :
				/* And they must not be keys of the object */
				if (tokens != NULL && parser->toksuper != -1) {
					jsmntok_t *t = &tokens[parser->toksuper];
					if (t->type == JSMN_OBJECT ||
					        (t->type == JSMN_STRING && t->size != 0)) {
						return JSMN_ERROR_INVAL;
					}
				}
#else
			/* In non-strict mode every unquoted value is a primitive */
			default:
#endif
				r = jsmn_parse_primitive(parser, js, len, tokens, num_tokens);
				if (r < 0) return r;
				count++;
				if (parser->toksuper != -1 && tokens != NULL)
					tokens[parser->toksuper].size++;
				break;

#ifdef JSMN_STRICT
			/* Unexpected char in strict mode */
			default:
				return JSMN_ERROR_INVAL;
#endif
		}
	}

	if (tokens != NULL) {
		for (i = parser->toknext - 1; i >= 0; i--) {
			/* Unmatched opened object or array */
			if (tokens[i].start != -1 && tokens[i].end == -1) {
				return JSMN_ERROR_PART;
			}
		}
	}

	return count;
}

/**
 * Creates a new parser based over a given  buffer with an array of tokens
 * available.
 */
void jsmn_init(jsmn_parser *parser) {
	parser->pos = 0;
	parser->toknext = 0;
	parser->toksuper = -1;
}
#include <stdio.h>

#define resize(sz) { \
	if (out->size + sz > out->allocated) { \
		LVChar *tmp = (LVChar *)malloc(out->allocated * 2); \
		memcpy(tmp, out->dest, out->size); \
		free(out->dest); \
		out->dest = tmp; \
		out->allocated = out->allocated * 2; \
	} \
}

#define add_single_token(token) { \
	memcpy(out->dest + out->size++, token, 1);\
}

#define add_new_token(token, sz) { \
	memcpy(out->dest + out->size, token, sz);\
	out->size += sz; \
}

struct jsonbuffer {
	LVChar *dest;
	LVInteger size;
	LVInteger allocated;
};

static int descent_level(VMHANDLE v, struct jsonbuffer *out) {
	const LVChar *s;
	switch (lv_gettype(v, -1)) {
		case OT_STRING:
			if (LV_SUCCEEDED(lv_tostring(v, -1))) {
				if (LV_SUCCEEDED(lv_getstring(v, -1, &s))) {
					LVInteger strsz = scstrlen(s);

					resize(strsz);

					add_single_token("\"");
					add_new_token(s, strsz);
					add_single_token("\"");

					lv_poptop(v);
					return 1;
				}
			}
			return 1;
		case OT_FLOAT:
		case OT_INTEGER:
		case OT_BOOL:
			if (LV_SUCCEEDED(lv_tostring(v, -1))) {
				if (LV_SUCCEEDED(lv_getstring(v, -1, &s))) {
					LVInteger strsz = scstrlen(s);

					resize(strsz);

					add_new_token(s, strsz);

					lv_poptop(v);
					return 1;
				}
			}
			return 1;
		case OT_NULL:
			resize(4);

			add_new_token("null", 4);
			return 1;
		case OT_TABLE: {
			resize(8);

			add_single_token("{");

			const LVChar *key;
			LVInteger count = lv_getsize(v, -1);
			lv_pushnull(v);
			while (LV_SUCCEEDED(lv_next(v, -2))) {
				lv_getstring(v, -2, &key);
				lv_remove(v, -2);
				lv_pushstring(v, key, -1);

				descent_level(v, out);
				lv_poptop(v);

				add_single_token(":");

				descent_level(v, out);
				lv_poptop(v);

				if (count > 1) {
					add_single_token(",");
				}

				count--;
			}

			add_single_token("}");

			lv_poptop(v);
			return 1;
		}
		case OT_ARRAY: {
			resize(8);

			add_single_token("[");

			LVInteger count = lv_getsize(v, -1);
			lv_pushnull(v);
			while (LV_SUCCEEDED(lv_next(v, -2))) {
				descent_level(v, out);

				if (count > 1) {
					add_single_token(",");
				}

				lv_pop(v, 2);
				count--;
			}

			add_single_token("]");

			lv_poptop(v);
			return 1;
		}
		default:
			return lv_throwerror(v, _LC("cannot convert type"));
	}

	return 0;
}

static LVInteger json_encode(VMHANDLE v) {
	struct jsonbuffer out;
	out.dest = (LVChar *)malloc(32);
	out.size = 0;
	out.allocated = 32;

	LVInteger res = descent_level(v, &out);
	if (!res) {
		free(out.dest);
		return 0;
	}

	out.dest[out.size] = '\0';
	lv_pushstring(v, out.dest, -1);
	free(out.dest);
	return 1;
}

static int parse_level(VMHANDLE v, const char *js, jsmntok_t *t, size_t count) {
	int i, j;
	if (count == 0) {
		return 0;
	}
	if (t->type == JSMN_PRIMITIVE) {
		LVChar tbuf[6];
		size_t tlen = t->end - t->start;
		if (tlen == 5) {
			strncpy(tbuf, js + t->start, tlen);
			tbuf[tlen] = '\0';
			for (size_t i = 0; i < tlen; ++i) {
				tbuf[i] = tolower(tbuf[i]);
			}
			if (!strcmp(tbuf, "false")) {
				lv_pushbool(v, LVFalse);
				return 1;
			}
		}
		if (tlen == 4) {
			strncpy(tbuf, js + t->start, tlen);
			tbuf[tlen] = '\0';
			for (size_t i = 0; i < tlen; ++i) {
				tbuf[i] = tolower(tbuf[i]);
			}
			if (!strcmp(tbuf, "true")) {
				lv_pushbool(v, LVTrue);
				return 1;
			} else if (!strcmp(tbuf, "null")) {
				lv_pushnull(v);
				return 1;
			}
		}
		const char *p = strchr(js + t->start, '.');
		if (!p) {
			lv_pushinteger(v, atol(js + t->start));
		} else {
			lv_pushfloat(v, atof(js + t->start));
		}
		return 1;
	} else if (t->type == JSMN_STRING) {
		lv_pushstring(v, js + t->start, t->end - t->start);
		return 1;
	} else if (t->type == JSMN_OBJECT) {
		j = 0;
		lv_newtable(v);
		for (i = 0; i < t->size; i++) {
			j += parse_level(v, js, t + 1 + j, count - j);
			j += parse_level(v, js, t + 1 + j, count - j);
			lv_rawset(v, -3);
		}
		return j + 1;
	} else if (t->type == JSMN_ARRAY) {
		j = 0;
		lv_newarray(v, 0);
		for (i = 0; i < t->size; i++) {
			j += parse_level(v, js, t + 1 + j, count - j);
			lv_arrayappend(v, -2);
		}
		return j + 1;
	}
	return 0;
}

static LVInteger json_decode(VMHANDLE v) {
	const LVChar *s;
	jsmn_parser p;
	size_t tokcount = 32;

	jsmn_init(&p);
	jsmntok_t *tok = (jsmntok_t *)lv_malloc(sizeof(*tok) * tokcount);
	if (LV_SUCCEEDED(lv_getstring(v, 2, &s))) {
try_again:
		int r = jsmn_parse(&p, s, scstrlen(s), tok, tokcount);
		if (r < 0) {
			if (r == JSMN_ERROR_NOMEM) {
				tokcount *= 2;
				tok = (jsmntok_t *)lv_realloc(tok, tokcount, sizeof(*tok) * tokcount);
				goto try_again;
			}

			return lv_throwerror(v, "Failed to parse as JSON");
		}

		parse_level(v, s, tok, p.toknext);

		lv_free(tok, tokcount);
		return 1;
	}
	return 0;
}

#define _DECL_FUNC(name,nparams,tycheck) {_LC(#name),name,nparams,tycheck}
static const LVRegFunction jsonlib_funcs[] = {
	_DECL_FUNC(json_encode, 2, NULL),
	_DECL_FUNC(json_decode, 2, _LC(".s")),
	{NULL, (LVFUNCTION)0, 0, NULL}
};
#undef _DECL_FUNC

LVRESULT mod_init_json(VMHANDLE v) {
	LVInteger i = 0;
	while (jsonlib_funcs[i].name != 0) {
		lv_pushstring(v, jsonlib_funcs[i].name, -1);
		lv_newclosure(v, jsonlib_funcs[i].f, 0);
		lv_setparamscheck(v, jsonlib_funcs[i].nparamscheck, jsonlib_funcs[i].typemask);
		lv_setnativeclosurename(v, -1, jsonlib_funcs[i].name);
		lv_newslot(v, -3, LVFalse);
		i++;
	}

	return LV_OK;
}
