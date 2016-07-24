#ifndef _SQSTD_STRING_H_
#define _SQSTD_STRING_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int SQRexBool;
typedef struct SQRex SQRex;

typedef struct {
	const SQChar *begin;
	SQInteger len;
} SQRexMatch;

LAVRIL_API SQRex *sqstd_rex_compile(const SQChar *pattern, const SQChar **error);
LAVRIL_API void sqstd_rex_free(SQRex *exp);
LAVRIL_API SQBool sqstd_rex_match(SQRex *exp, const SQChar *text);
LAVRIL_API SQBool sqstd_rex_search(SQRex *exp, const SQChar *text, const SQChar **out_begin, const SQChar **out_end);
LAVRIL_API SQBool sqstd_rex_searchrange(SQRex *exp, const SQChar *text_begin, const SQChar *text_end, const SQChar **out_begin, const SQChar **out_end);
LAVRIL_API SQInteger sqstd_rex_getsubexpcount(SQRex *exp);
LAVRIL_API SQBool sqstd_rex_getsubexp(SQRex *exp, SQInteger n, SQRexMatch *subexp);

LAVRIL_API SQRESULT sqstd_format(HSQUIRRELVM v, SQInteger nformatstringidx, SQInteger *outlen, SQChar **output);

LAVRIL_API SQRESULT sqstd_register_stringlib(HSQUIRRELVM v);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*_SQSTD_STRING_H_*/
