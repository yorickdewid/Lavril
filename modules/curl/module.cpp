#include <lavril.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <setjmp.h>

#define USERAGENT "libcurl-lavril/1.0"

struct cbbuffer {
	char *buf;
	size_t size;
};

struct LVCURLObj {
	CURL *_curl;
	CURLcode res;
	const LVChar *_url;
	LVInteger _nallocated;
	struct curl_slist *_hlist;
	void *_jmpbuf;
	const LVChar **_error;
	struct cbbuffer _chunk;
};

const int mod_curl_opt[] = {
	CURLOPT_POST,
	// CURLOPT_PUT,
	// CURLOPT_REFERER,
	// CURLOPT_POSTFIELDS,
	// CURLOPT_REFERER,
	// CURLOPT_FOLLOWLOCATION,
	-1,
};

#if 0
void register_curl_setopt(VMHANDLE v) {
	LVInteger i = 0;
	// lv_pushstring(v, _LC("CURLOPT_POST"), -1);
	// lv_pushinteger(v, CURLOPT_POST);
	// lv_newslot(v, -3, LVFalse);

	while (mod_curl_opt[i] != 0) {
		lv_pushstring(v, _LC("CURLOPT_POSTFIELDS"), -1);
		lv_pushinteger(v, CURLOPT_POSTFIELDS);
		lv_newslot(v, -3, LVFalse);
		i++;
	}
}
#endif

#define OBJECT_INSTANCE(v) \
	LVCURLObj *self = NULL; \
	lv_getinstanceup(v,1,(LVUserPointer *)&self, 0);

void mod_curl_free(LVCURLObj *exp) {
	if (exp) {
		curl_easy_cleanup(exp->_curl);
		if (exp->_jmpbuf)
			lv_free(exp->_jmpbuf, sizeof(jmp_buf));
		if (exp->_chunk.buf)
			lv_free(exp->_chunk.buf, 1);
		if (exp->_hlist)
			lv_free(exp->_hlist, 0);
		lv_free(exp, sizeof(LVCURLObj));
		curl_global_cleanup();
	}
}

static void mod_curl_error(LVCURLObj *exp, const LVChar *error) {
	if (exp->_error)
		*exp->_error = error;
	longjmp(*((jmp_buf *)exp->_jmpbuf), -1);
}

static LVInteger mod_curl_releasehook(LVUserPointer p, LVInteger LV_UNUSED_ARG(size)) {
	LVCURLObj *self = ((LVCURLObj *)p);
	mod_curl_free(self);
	return 1;
}

static void init_buffer(struct cbbuffer *s) {
	s->size = 0;
	s->buf = (char *)lv_malloc(s->size + 1);
	s->buf[0] = '\0';
}

static size_t callback_buffer(void *ptr, size_t size, size_t nmemb, struct cbbuffer *s) {
	size_t new_len = s->size + size * nmemb;
	s->buf = (char *)lv_realloc(s->buf, 0, new_len + 1);
	if (!s->buf) {
		return 0;
	}
	memcpy(s->buf + s->size, ptr, size * nmemb);
	s->buf[new_len] = '\0';
	s->size = new_len;

	return size * nmemb;
}

LVCURLObj *mod_curl_init(const LVChar *location, const LVChar **error) {
	LVCURLObj *volatile exp = (LVCURLObj *)lv_malloc(sizeof(LVCURLObj));
	exp->_curl = NULL;
	exp->_url = location;
	exp->_nallocated = (LVInteger)scstrlen(location) * sizeof(LVChar);
	exp->_hlist = NULL;
	exp->_error = error;
	exp->_jmpbuf = lv_malloc(sizeof(jmp_buf));
	init_buffer(&exp->_chunk);
	if (setjmp(*((jmp_buf *)exp->_jmpbuf)) == 0) {
		exp->_curl = curl_easy_init();
		if (!exp->_curl)
			mod_curl_error(exp, _LC("initialization failed"));


		/* Default options */
		curl_easy_setopt(exp->_curl, CURLOPT_URL, exp->_url);
		curl_easy_setopt(exp->_curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(exp->_curl, CURLOPT_USERAGENT, USERAGENT);

		/* Write callback */
		curl_easy_setopt(exp->_curl, CURLOPT_WRITEFUNCTION, callback_buffer);
		curl_easy_setopt(exp->_curl, CURLOPT_WRITEDATA, (void *)&exp->_chunk);
	} else {
		mod_curl_free(exp);
		return NULL;
	}
	return exp;
}

#if 0
static LVInteger _curl_setopt(VMHANDLE v) {
	OBJECT_INSTANCE(v);
	const LVChar *str;
	LVInteger opt = 0;
	lv_getinteger(v, 2, &opt);
	lv_getstring(v, 3, &str);

	// curl_easy_setopt(self->_curl, CURLOPT_VERBOSE, 1L);

	printf("opt %lld, str %s\n", opt, str);
	/*if (sqstd_rex_search(self, str + start, &begin, &end) == LVTrue) {
		LVInteger n = sqstd_rex_getsubexpcount(self);
		SQRexMatch match;
		sq_newarray(v, 0);
		for (LVInteger i = 0; i < n; i++) {
			sqstd_rex_getsubexp(self, i, &match);
			if (match.len > 0)
				_addrexmatch(v, str, match.begin, match.begin + match.len);
			else
				_addrexmatch(v, str, str, str); //empty match
			sq_arrayappend(v, -2);
		}
		return 1;
	}*/
	return 0;
}
#endif

static LVInteger _curl_setheader(VMHANDLE v) {
	OBJECT_INSTANCE(v);
	const LVChar *key, *value;
	lv_getstring(v, 2, &key);
	lv_getstring(v, 3, &value);
	LVChar *temp = lv_getscratchpad(v, scstrlen(key) + scstrlen(value) + 3);
	scstrcpy(temp, key);
	scstrcat(temp, ": ");
	scstrcat(temp, value);
	self->_hlist = curl_slist_append(self->_hlist, temp);
	return 0;
}

static LVInteger _curl_exec(VMHANDLE v) {
	OBJECT_INSTANCE(v);
	if (self->_hlist) {
		self->res = curl_easy_setopt(self->_curl, CURLOPT_HTTPHEADER, self->_hlist);
		if (self->res != CURLE_OK)
			mod_curl_error(self, curl_easy_strerror(self->res));
	}
	self->res = curl_easy_perform(self->_curl);
	if (self->res != CURLE_OK)
		mod_curl_error(self, curl_easy_strerror(self->res));
	lv_pushstring(v, self->_chunk.buf, self->_chunk.size);
	return 1;
}

static LVInteger _curl_constructor(VMHANDLE v) {
	const LVChar *error, *location;
	lv_getstring(v, 2, &location);
	LVCURLObj *curl = mod_curl_init(location, &error);
	if (!curl)
		return lv_throwerror(v, error);
	lv_setinstanceup(v, 1, curl);
	lv_setreleasehook(v, 1, mod_curl_releasehook);
	return 0;
}

static LVInteger _curl__typeof(VMHANDLE v) {
	lv_pushstring(v, _LC("curl"), -1);
	return 1;
}

#define _DECL_CURL_FUNC(name,nparams,pmask) {_LC(#name),_curl_##name,nparams,pmask}
static const SQRegFunction curlobj_funcs[] = {
	_DECL_CURL_FUNC(constructor, 2, _LC(".s")),
	_DECL_CURL_FUNC(exec, 1, _LC("x")),
	// _DECL_CURL_FUNC(setopt, 3, _LC("xns")),
	_DECL_CURL_FUNC(setheader, 3, _LC("xss")),
	_DECL_CURL_FUNC(_typeof, 1, _LC("x")),
	{NULL, (SQFUNCTION)0, 0, NULL}
};
#undef _DECL_CURL_FUNC

LVInteger mod_init_curl(VMHANDLE v) {
	LVInteger i = 0;

	lv_pushstring(v, _LC("curl"), -1);
	lv_newclass(v, LVFalse);
	while (curlobj_funcs[i].name != 0) {
		const SQRegFunction& f = curlobj_funcs[i];
		lv_pushstring(v, f.name, -1);
		lv_newclosure(v, f.f, 0);
		lv_setparamscheck(v, f.nparamscheck, f.typemask);
		lv_setnativeclosurename(v, -1, f.name);
		lv_newslot(v, -3, LVFalse);
		i++;
	}
	lv_newslot(v, -3, LVFalse);
	// register_curl_setopt(v);
	return 1;
}
