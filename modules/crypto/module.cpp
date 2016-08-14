#include <lavril.h>
#include <stdlib.h>
#include <string.h>
#include "sha1.h"
#include "base64.h"
#include "urlcode.h"

static LVInteger crypto_base64_encode(VMHANDLE v) {
	size_t out = 0;
	const LVChar *s;
	if (LV_SUCCEEDED(lv_getstring(v, 2, &s))) {
		char *encoded = base64_encode((const unsigned char *)s, scstrlen(s), &out);
		lv_pushstring(v, encoded, out);
		free(encoded);
		return 1;
	}
	return 0;
}

static LVInteger crypto_base64_decode(VMHANDLE v) {
	size_t out = 0;
	const LVChar *s;
	if (LV_SUCCEEDED(lv_getstring(v, 2, &s))) {
		unsigned char *decoded = base64_decode((const char *)s, scstrlen(s), &out);
		lv_pushstring(v, (const LVChar *)decoded, out);
		free(decoded);
		return 1;
	}
	return 0;
}

static LVInteger crypto_url_encode(VMHANDLE v) {
	const LVChar *s;
	if (LV_SUCCEEDED(lv_getstring(v, 2, &s))) {
		char *encoded = url_encode(s);
		lv_pushstring(v, encoded, -1);
		free(encoded);
		return 1;
	}
	return 0;
}

static LVInteger crypto_url_decode(VMHANDLE v) {
	const LVChar *s;
	if (LV_SUCCEEDED(lv_getstring(v, 2, &s))) {
		char *encoded = url_decode(s);
		lv_pushstring(v, encoded, -1);
		free(encoded);
		return 1;
	}
	return 0;
}

static LVInteger crypto_sha1(VMHANDLE v) {
	SHA1 sha1;
	const LVChar *s;
	if (LV_SUCCEEDED(lv_getstring(v, 2, &s))) {
		sha1.update(s);
		lv_pushstring(v, sha1.final().c_str(), -1);
		return 1;
	}
	return 0;
}

#define _DECL_FUNC(name,nparams,tycheck) {_LC(#name),crypto_##name,nparams,tycheck}
static const LVRegFunction cryptolib_funcs[] = {
	_DECL_FUNC(base64_encode, 2, _LC(".s")),
	_DECL_FUNC(base64_decode, 2, _LC(".s")),
	_DECL_FUNC(url_encode, 2, _LC(".s")),
	_DECL_FUNC(url_decode, 2, _LC(".s")),
	_DECL_FUNC(sha1, 2, _LC(".s")),
	{NULL, (LVFUNCTION)0, 0, NULL}
};
#undef _DECL_FUNC

LVRESULT mod_init_crypto(VMHANDLE v) {
	LVInteger i = 0;
	while (cryptolib_funcs[i].name != 0) {
		lv_pushstring(v, cryptolib_funcs[i].name, -1);
		lv_newclosure(v, cryptolib_funcs[i].f, 0);
		lv_setparamscheck(v, cryptolib_funcs[i].nparamscheck, cryptolib_funcs[i].typemask);
		lv_setnativeclosurename(v, -1, cryptolib_funcs[i].name);
		lv_newslot(v, -3, LVFalse);
		i++;
	}

	return LV_OK;
}
