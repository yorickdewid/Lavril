#include <lavril.h>
#include <math.h>
#include <stdlib.h>

#define SINGLE_ARG_FUNC(_funcname) static SQInteger math_##_funcname(HSQUIRRELVM v){ \
	SQFloat f; \
	lv_getfloat(v,2,&f); \
	lv_pushfloat(v,(SQFloat)_funcname(f)); \
	return 1; \
}

#define TWO_ARGS_FUNC(_funcname) static SQInteger math_##_funcname(HSQUIRRELVM v){ \
	SQFloat p1,p2; \
	lv_getfloat(v,2,&p1); \
	lv_getfloat(v,3,&p2); \
	lv_pushfloat(v,(SQFloat)_funcname(p1,p2)); \
	return 1; \
}

static SQInteger math_srand(HSQUIRRELVM v) {
	SQInteger i;
	if (LV_FAILED(lv_getinteger(v, 2, &i)))
		return lv_throwerror(v, _SC("invalid param"));
	srand((unsigned int)i);
	return 0;
}

static SQInteger math_rand(HSQUIRRELVM v) {
	lv_pushinteger(v, rand());
	return 1;
}

static SQInteger math_abs(HSQUIRRELVM v) {
	SQInteger n;
	lv_getinteger(v, 2, &n);
	lv_pushinteger(v, (SQInteger)abs((int)n));
	return 1;
}

static SQInteger math_universe(HSQUIRRELVM v) {
	lv_pushinteger(v, 42);
	return 1;
}

SINGLE_ARG_FUNC(sqrt)
SINGLE_ARG_FUNC(fabs)
SINGLE_ARG_FUNC(sin)
SINGLE_ARG_FUNC(cos)
SINGLE_ARG_FUNC(asin)
SINGLE_ARG_FUNC(acos)
SINGLE_ARG_FUNC(log)
SINGLE_ARG_FUNC(log10)
SINGLE_ARG_FUNC(tan)
SINGLE_ARG_FUNC(atan)
TWO_ARGS_FUNC(atan2)
TWO_ARGS_FUNC(pow)
SINGLE_ARG_FUNC(floor)
SINGLE_ARG_FUNC(ceil)
SINGLE_ARG_FUNC(exp)

#define _DECL_FUNC(name,nparams,tycheck) {_SC(#name),math_##name,nparams,tycheck}
static const SQRegFunction mathlib_funcs[] = {
	_DECL_FUNC(sqrt, 2, _SC(".n")),
	_DECL_FUNC(sin, 2, _SC(".n")),
	_DECL_FUNC(cos, 2, _SC(".n")),
	_DECL_FUNC(asin, 2, _SC(".n")),
	_DECL_FUNC(acos, 2, _SC(".n")),
	_DECL_FUNC(log, 2, _SC(".n")),
	_DECL_FUNC(log10, 2, _SC(".n")),
	_DECL_FUNC(tan, 2, _SC(".n")),
	_DECL_FUNC(atan, 2, _SC(".n")),
	_DECL_FUNC(atan2, 3, _SC(".nn")),
	_DECL_FUNC(pow, 3, _SC(".nn")),
	_DECL_FUNC(floor, 2, _SC(".n")),
	_DECL_FUNC(ceil, 2, _SC(".n")),
	_DECL_FUNC(exp, 2, _SC(".n")),
	_DECL_FUNC(srand, 2, _SC(".n")),
	_DECL_FUNC(rand, 1, NULL),
	_DECL_FUNC(fabs, 2, _SC(".n")),
	_DECL_FUNC(abs, 2, _SC(".n")),
	_DECL_FUNC(universe, 1, _SC(".n")),
	{NULL, (SQFUNCTION)0, 0, NULL}
};
#undef _DECL_FUNC

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

SQRESULT mod_init_math(HSQUIRRELVM v) {
	SQInteger i = 0;
	while (mathlib_funcs[i].name != 0) {
		lv_pushstring(v, mathlib_funcs[i].name, -1);
		lv_newclosure(v, mathlib_funcs[i].f, 0);
		lv_setparamscheck(v, mathlib_funcs[i].nparamscheck, mathlib_funcs[i].typemask);
		lv_setnativeclosurename(v, -1, mathlib_funcs[i].name);
		lv_newslot(v, -3, SQFalse);
		i++;
	}

	lv_pushstring(v, _SC("RAND_MAX"), -1);
	lv_pushinteger(v, RAND_MAX);
	lv_newslot(v, -3, SQFalse);
	lv_pushstring(v, _SC("PI"), -1);
	lv_pushfloat(v, (SQFloat)M_PI);
	lv_newslot(v, -3, SQFalse);

	return LV_OK;
}
