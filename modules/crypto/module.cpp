#include <lavril.h>
#include <stdlib.h>
#include "sha1.h"

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
