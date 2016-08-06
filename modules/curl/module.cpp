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
	const SQChar *_url;
	SQInteger _op;
	SQInteger _nallocated;
	SQInteger _nsize;
	SQInteger _nsubexpr;
	void *_jmpbuf;
	const SQChar **_error;
	struct cbbuffer _chunk;
};

#define OBJECT_INSTANCE(v) \
	LVCURLObj *self = NULL; \
	lv_getinstanceup(v,1,(SQUserPointer *)&self, 0);

void mod_curl_free(LVCURLObj *exp) {
	if (exp) {
		curl_easy_cleanup(exp->_curl);
		if (exp->_jmpbuf)
			lv_free(exp->_jmpbuf, sizeof(jmp_buf));
		if (exp->_chunk.buf)
			lv_free(exp->_chunk.buf, 1);
		lv_free(exp, sizeof(LVCURLObj));
	}
}

static void mod_curl_error(LVCURLObj *exp, const SQChar *error) {
	if (exp->_error)
		*exp->_error = error;
	longjmp(*((jmp_buf *)exp->_jmpbuf), -1);
}

static SQInteger mod_curl_releasehook(SQUserPointer p, SQInteger LV_UNUSED_ARG(size)) {
	LVCURLObj *self = ((LVCURLObj *)p);
	mod_curl_free(self);
	return 1;
}

static void init_buffer(struct cbbuffer *s) {
	s->size = 0;
	s->buf = (char *)lv_malloc(s->size + 1);
	if (s->buf == NULL) {
		fprintf(stderr, "malloc() failed\n");
		exit(EXIT_FAILURE);
	}
	s->buf[0] = '\0';
}

static size_t callback_buffer(void *ptr, size_t size, size_t nmemb, struct cbbuffer *s) {
	size_t new_len = s->size + size * nmemb;
	s->buf = (char *)lv_realloc(s->buf, 0, new_len + 1);
	if (!s->buf) {
		fprintf(stderr, "realloc() failed\n");
		exit(EXIT_FAILURE);
	}
	memcpy(s->buf + s->size, ptr, size * nmemb);
	s->buf[new_len] = '\0';
	s->size = new_len;

	return size * nmemb;
}

LVCURLObj *mod_curl_init(const SQChar *location, const SQChar **error) {
	LVCURLObj *volatile exp = (LVCURLObj *)lv_malloc(sizeof(LVCURLObj));
	exp->_curl = NULL;
	exp->_url = location;
	exp->_nallocated = (SQInteger)scstrlen(location) * sizeof(SQChar);
	exp->_nsize = 0;
	exp->_nsubexpr = 0;
	exp->_error = error;
	exp->_jmpbuf = lv_malloc(sizeof(jmp_buf));
	init_buffer(&exp->_chunk);
	if (setjmp(*((jmp_buf *)exp->_jmpbuf)) == 0) {
		exp->_curl = curl_easy_init();
		if (!exp->_curl)
			mod_curl_error(exp, _SC("initialization failed"));


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

static SQInteger _curl_exec(HSQUIRRELVM v) {
	OBJECT_INSTANCE(v);
	self->res = curl_easy_perform(self->_curl);
	if (self->res != CURLE_OK)
		mod_curl_error(self, curl_easy_strerror(self->res));
	lv_pushstring(v, self->_chunk.buf, self->_chunk.size);
	return 1;
}

static SQInteger _curl_constructor(HSQUIRRELVM v) {
	const SQChar *error, *location;
	lv_getstring(v, 2, &location);
	LVCURLObj *curl = mod_curl_init(location, &error);
	if (!curl)
		return lv_throwerror(v, error);
	lv_setinstanceup(v, 1, curl);
	lv_setreleasehook(v, 1, mod_curl_releasehook);
	return 0;
}

static SQInteger _curl__typeof(HSQUIRRELVM v) {
	lv_pushstring(v, _SC("curl"), -1);
	return 1;
}

#define _DECL_CURL_FUNC(name,nparams,pmask) {_SC(#name),_curl_##name,nparams,pmask}
static const SQRegFunction curlobj_funcs[] = {
	_DECL_CURL_FUNC(constructor, 2, _SC(".s")),
	_DECL_CURL_FUNC(exec, 1, _SC("x")),
	_DECL_CURL_FUNC(_typeof, 1, _SC("x")),
	{NULL, (SQFUNCTION)0, 0, NULL}
};
#undef _DECL_CURL_FUNC

SQInteger mod_init_curl(HSQUIRRELVM v) {
	SQInteger i = 0;

	lv_pushstring(v, _SC("curl"), -1);
	lv_newclass(v, SQFalse);
	while (curlobj_funcs[i].name != 0) {
		const SQRegFunction& f = curlobj_funcs[i];
		lv_pushstring(v, f.name, -1);
		lv_newclosure(v, f.f, 0);
		lv_setparamscheck(v, f.nparamscheck, f.typemask);
		lv_setnativeclosurename(v, -1, f.name);
		lv_newslot(v, -3, SQFalse);
		i++;
	}
	lv_newslot(v, -3, SQFalse);
	return 1;
}
