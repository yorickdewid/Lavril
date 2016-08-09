#include <lavril.h>
#include <stdlib.h>
#include <string.h>
#include "sha1.h"
#include "base64.h"
#include "urlcode.h"

static SQInteger crypto_base64_encode(HSQUIRRELVM v) {
	size_t out = 0;
	const SQChar *s;
	if (LV_SUCCEEDED(lv_getstring(v, 2, &s))) {
		char *encoded = base64_encode((const unsigned char *)s, scstrlen(s), &out);
		lv_pushstring(v, encoded, out);
		free(encoded);
		return 1;
	}
	return 0;
}

static SQInteger crypto_base64_decode(HSQUIRRELVM v) {
	size_t out = 0;
	const SQChar *s;
	if (LV_SUCCEEDED(lv_getstring(v, 2, &s))) {
		unsigned char *decoded = base64_decode((const char *)s, scstrlen(s), &out);
		lv_pushstring(v, (const SQChar *)decoded, out);
		free(decoded);
		return 1;
	}
	return 0;
}

static SQInteger crypto_url_encode(HSQUIRRELVM v) {
	const SQChar *s;
	if (LV_SUCCEEDED(lv_getstring(v, 2, &s))) {
		char *encoded = url_encode(s);
		lv_pushstring(v, encoded, -1);
		free(encoded);
		return 1;
	}
	return 0;
}

static SQInteger crypto_url_decode(HSQUIRRELVM v) {
	const SQChar *s;
	if (LV_SUCCEEDED(lv_getstring(v, 2, &s))) {
		char *encoded = url_decode(s);
		lv_pushstring(v, encoded, -1);
		free(encoded);
		return 1;
	}
	return 0;
}

static SQInteger crypto_sha1(HSQUIRRELVM v) {
	SHA1 sha1;
	const SQChar *s;
	if (LV_SUCCEEDED(lv_getstring(v, 2, &s))) {
		sha1.update(s);
		lv_pushstring(v, sha1.final().c_str(), -1);
		return 1;
	}
	return 0;
}

#define _DECL_FUNC(name,nparams,tycheck) {_SC(#name),crypto_##name,nparams,tycheck}
static const SQRegFunction cryptolib_funcs[] = {
	_DECL_FUNC(base64_encode, 2, _SC(".s")),
	_DECL_FUNC(base64_decode, 2, _SC(".s")),
	_DECL_FUNC(url_encode, 2, _SC(".s")),
	_DECL_FUNC(url_decode, 2, _SC(".s")),
	_DECL_FUNC(sha1, 2, _SC(".s")),
	{NULL, (SQFUNCTION)0, 0, NULL}
};
#undef _DECL_FUNC

SQRESULT mod_init_crypto(HSQUIRRELVM v) {
	SQInteger i = 0;
	while (cryptolib_funcs[i].name != 0) {
		lv_pushstring(v, cryptolib_funcs[i].name, -1);
		lv_newclosure(v, cryptolib_funcs[i].f, 0);
		lv_setparamscheck(v, cryptolib_funcs[i].nparamscheck, cryptolib_funcs[i].typemask);
		lv_setnativeclosurename(v, -1, cryptolib_funcs[i].name);
		lv_newslot(v, -3, SQFalse);
		i++;
	}

	return LV_OK;
}
