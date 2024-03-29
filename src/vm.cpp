#include "pcheader.h"
#include <math.h>
#include <stdlib.h>
#include "opcodes.h"
#include "vm.h"
#include "funcproto.h"
#include "closure.h"
#include "lvstring.h"
#include "table.h"
#include "userdata.h"
#include "array.h"
#include "class.h"

#define TOP() (_stack._vals[_top-1])

bool LVVM::BW_OP(LVUnsignedInteger op, LVObjectPtr& trg, const LVObjectPtr& o1, const LVObjectPtr& o2) {
	LVInteger res;
	if ((type(o1) | type(o2)) == OT_INTEGER) {
		LVInteger i1 = _integer(o1), i2 = _integer(o2);
		switch (op) {
			case BW_AND:
				res = i1 & i2;
				break;
			case BW_OR:
				res = i1 | i2;
				break;
			case BW_XOR:
				res = i1 ^ i2;
				break;
			case BW_SHIFTL:
				res = i1 << i2;
				break;
			case BW_SHIFTR:
				res = i1 >> i2;
				break;
			case BW_USHIFTR:
				res = (LVInteger)(*((LVUnsignedInteger *)&i1) >> i2);
				break;
			default: {
				Raise_Error(_LC("internal vm error bitwise op failed"));
				return false;
			}
		}
	} else {
		Raise_Error(_LC("bitwise op between '%s' and '%s'"), GetTypeName(o1), GetTypeName(o2));
		return false;
	}
	trg = res;
	return true;
}

#define _ARITH_(op,trg,o1,o2) \
{ \
	LVInteger tmask = type(o1)|type(o2); \
	switch(tmask) { \
		case OT_INTEGER: trg = _integer(o1) op _integer(o2);break; \
		case (OT_FLOAT|OT_INTEGER): \
		case (OT_FLOAT): trg = tofloat(o1) op tofloat(o2); break;\
		default: _GUARD(ARITH_OP((#op)[0],trg,o1,o2)); break;\
	} \
}

#define _ARITH_NOZERO(op,trg,o1,o2,err) \
{ \
	LVInteger tmask = type(o1)|type(o2); \
	switch(tmask) { \
		case OT_INTEGER: { LVInteger i2 = _integer(o2); if(i2 == 0) { Raise_Error(err); THROW(); } trg = _integer(o1) op i2; } break;\
		case (OT_FLOAT|OT_INTEGER): \
		case (OT_FLOAT): trg = tofloat(o1) op tofloat(o2); break;\
		default: _GUARD(ARITH_OP((#op)[0],trg,o1,o2)); break;\
	} \
}

bool LVVM::ARITH_OP(LVUnsignedInteger op, LVObjectPtr& trg, const LVObjectPtr& o1, const LVObjectPtr& o2) {
	LVInteger tmask = type(o1) | type(o2);
	switch (tmask) {
		case OT_INTEGER: {
			LVInteger res, i1 = _integer(o1), i2 = _integer(o2);
			switch (op) {
				case '+':
					res = i1 + i2;
					break;
				case '-':
					res = i1 - i2;
					break;
				case '/':
					if (i2 == 0) {
						Raise_Error(_LC("division by zero"));
						return false;
					} else if (i2 == -1 && i1 == INT_MIN) {
						Raise_Error(_LC("integer overflow"));
						return false;
					}
					res = i1 / i2;
					break;
				case '*':
					res = i1 * i2;
					break;
				case '%':
					if (i2 == 0) {
						Raise_Error(_LC("modulo by zero"));
						return false;
					} else if (i2 == -1 && i1 == INT_MIN) {
						res = 0;
						break;
					}
					res = i1 % i2;
					break;
				default:
					res = 0xDEADBEEF;
			}
			trg = res;
		}
		break;
		case (OT_FLOAT|OT_INTEGER):
		case (OT_FLOAT): {
			LVFloat res, f1 = tofloat(o1), f2 = tofloat(o2);
			switch (op) {
				case '+':
					res = f1 + f2;
					break;
				case '-':
					res = f1 - f2;
					break;
				case '/':
					res = f1 / f2;
					break;
				case '*':
					res = f1 * f2;
					break;
				case '%':
					res = LVFloat(fmod((double)f1, (double)f2));
					break;
				default:
					res = 0x0f;
			}
			trg = res;
		}
		break;
		default:
			if (op == '+' && (tmask & _RT_STRING)) {
				if (!StringCat(o1, o2, trg)) return false;
			} else if (!ArithMetaMethod(op, o1, o2, trg)) {
				return false;
			}
	}
	return true;
}

LVVM::LVVM(LVSharedState *ss) {
	_sharedstate = ss;
	_suspended = LVFalse;
	_suspended_target = -1;
	_suspended_root = LVFalse;
	_suspended_traps = -1;
	_foreignptr = NULL;
	_nnativecalls = 0;
	_nmetamethodscall = 0;
	_lasterror.Null();
	_errorhandler.Null();
	_debughook = false;
	_debughook_native = NULL;
	_debughook_closure.Null();
	_openouters = NULL;
	ci = NULL;
	_releasehook = NULL;
	INIT_CHAIN();
	ADD_TO_CHAIN(&_ss(this)->_gc_chain, this);
}

void LVVM::Finalize() {
	if (_releasehook) {
		_releasehook(_foreignptr, 0);
		_releasehook = NULL;
	}
	if (_openouters)
		CloseOuters(&_stack._vals[0]);
	_roottable.Null();
	_lasterror.Null();
	_errorhandler.Null();
	_debughook = false;
	_debughook_native = NULL;
	_debughook_closure.Null();
	temp_reg.Null();
	_callstackdata.resize(0);
	LVInteger size = _stack.size();
	for (LVInteger i = 0; i < size; i++)
		_stack[i].Null();
}

LVVM::~LVVM() {
	Finalize();
	REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain, this);
}

bool LVVM::ArithMetaMethod(LVInteger op, const LVObjectPtr& o1, const LVObjectPtr& o2, LVObjectPtr& dest) {
	LVMetaMethod mm;
	switch (op) {
		case _LC('+'):
			mm = MT_ADD;
			break;
		case _LC('-'):
			mm = MT_SUB;
			break;
		case _LC('/'):
			mm = MT_DIV;
			break;
		case _LC('*'):
			mm = MT_MUL;
			break;
		case _LC('%'):
			mm = MT_MODULO;
			break;
		default:
			mm = MT_ADD;
			assert(0);
			break; //shutup compiler
	}
	if (is_delegable(o1) && _delegable(o1)->_delegate) {
		LVObjectPtr closure;
		if (_delegable(o1)->GetMetaMethod(this, mm, closure)) {
			Push(o1);
			Push(o2);
			return CallMetaMethod(closure, mm, 2, dest);
		}
	}

	Raise_Error(_LC("arith op %c on between '%s' and '%s'"), op, GetTypeName(o1), GetTypeName(o2));
	return false;
}

bool LVVM::NEG_OP(LVObjectPtr& trg, const LVObjectPtr& o) {
	switch (type(o)) {
		case OT_INTEGER:
			trg = -_integer(o);
			return true;
		case OT_FLOAT:
			trg = -_float(o);
			return true;
		case OT_TABLE:
		case OT_USERDATA:
		case OT_INSTANCE:
			if (_delegable(o)->_delegate) {
				LVObjectPtr closure;
				if (_delegable(o)->GetMetaMethod(this, MT_UNM, closure)) {
					Push(o);
					if (!CallMetaMethod(closure, MT_UNM, 1, temp_reg)) return false;
					_Swap(trg, temp_reg);
					return true;

				}
			}
		default:
			break; //shutup compiler
	}
	Raise_Error(_LC("attempt to negate a %s"), GetTypeName(o));
	return false;
}

#define _RET_SUCCEED(exp) { result = (exp); return true; }
bool LVVM::ObjCmp(const LVObjectPtr& o1, const LVObjectPtr& o2, LVInteger& result) {
	LVObjectType t1 = type(o1), t2 = type(o2);
	if (t1 == t2) {
		if (_rawval(o1) == _rawval(o2))_RET_SUCCEED(0);
		LVObjectPtr res;
		switch (t1) {
			case OT_STRING:
				_RET_SUCCEED(scstrcmp(_stringval(o1), _stringval(o2)));
			case OT_INTEGER:
				_RET_SUCCEED((_integer(o1) < _integer(o2)) ? -1 : 1);
			case OT_FLOAT:
				_RET_SUCCEED((_float(o1) < _float(o2)) ? -1 : 1);
			case OT_TABLE:
			case OT_USERDATA:
			case OT_INSTANCE:
				if (_delegable(o1)->_delegate) {
					LVObjectPtr closure;
					if (_delegable(o1)->GetMetaMethod(this, MT_CMP, closure)) {
						Push(o1);
						Push(o2);
						if (CallMetaMethod(closure, MT_CMP, 2, res)) {
							if (type(res) != OT_INTEGER) {
								Raise_Error(_LC("_cmp must return an integer"));
								return false;
							}
							_RET_SUCCEED(_integer(res))
						}
						return false;
					}
				}
			//continues through (no break needed)
			default:
				_RET_SUCCEED( _userpointer(o1) < _userpointer(o2) ? -1 : 1 );
		}
		assert(0);
		//if(type(res)!=OT_INTEGER) { Raise_CompareError(o1,o2); return false; }
		//  _RET_SUCCEED(_integer(res));

	} else {
		if (lv_isnumeric(o1) && lv_isnumeric(o2)) {
			if ((t1 == OT_INTEGER) && (t2 == OT_FLOAT)) {
				if ( _integer(o1) == _float(o2) ) {
					_RET_SUCCEED(0);
				} else if ( _integer(o1) < _float(o2) ) {
					_RET_SUCCEED(-1);
				}
				_RET_SUCCEED(1);
			} else {
				if ( _float(o1) == _integer(o2) ) {
					_RET_SUCCEED(0);
				} else if ( _float(o1) < _integer(o2) ) {
					_RET_SUCCEED(-1);
				}
				_RET_SUCCEED(1);
			}
		} else if (t1 == OT_NULL) {
			_RET_SUCCEED(-1);
		} else if (t2 == OT_NULL) {
			_RET_SUCCEED(1);
		} else {
			Raise_CompareError(o1, o2);
			return false;
		}

	}
	assert(0);
	_RET_SUCCEED(0); //cannot happen
}

bool LVVM::CMP_OP(CmpOP op, const LVObjectPtr& o1, const LVObjectPtr& o2, LVObjectPtr& res) {
	LVInteger r;
	if (ObjCmp(o1, o2, r)) {
		switch (op) {
			case CMP_G:
				res = (r > 0);
				return true;
			case CMP_GE:
				res = (r >= 0);
				return true;
			case CMP_L:
				res = (r < 0);
				return true;
			case CMP_LE:
				res = (r <= 0);
				return true;
			case CMP_3W:
				res = r;
				return true;
		}
		assert(0);
	}
	return false;
}

bool LVVM::ToString(const LVObjectPtr& o, LVObjectPtr& res, LVInteger ident) {
	switch (type(o)) {
		case OT_STRING:
			res = o;
			return true;
		case OT_FLOAT:
			scsprintf(_sp(lv_rsl(NUMBER_MAX_CHAR + 1)), lv_rsl(NUMBER_MAX_CHAR), _LC("%g"), _float(o));
			break;
		case OT_INTEGER:
			scsprintf(_sp(lv_rsl(NUMBER_MAX_CHAR + 1)), lv_rsl(NUMBER_MAX_CHAR), _PRINT_INT_FMT, _integer(o));
			break;
		case OT_BOOL:
			scsprintf(_sp(lv_rsl(6)), lv_rsl(6), _integer(o) ? _LC("true") : _LC("false"));
			break;
		case OT_TABLE: {
			LVObjectPtr refidx, key, val, rs, str = LVString::Create(_ss(this), _LC("(table) {\n"));
			LVInteger idx;

			/* TODO: optimize */
			while ((idx = _table(o)->Next(false, refidx, key, val)) != -1) {
				refidx = idx;
				this->ToString(val, rs, ident + 1);
				for (int i = 0; i < ident + 1; ++i) {
					this->StringCat(str, LVString::Create(_ss(this), _LC("  ")), str);
				}
				this->StringCat(str, key, str);
				this->StringCat(str, LVString::Create(_ss(this), _LC(" = ")), str);
				this->StringCat(str, rs, str);
				this->StringCat(str, LVString::Create(_ss(this), _LC(",\n")), str);
			}

			if (!_table(o)->CountUsed()) {
				for (int i = 0; i < ident + 1; ++i) {
					this->StringCat(str, LVString::Create(_ss(this), _LC("  ")), str);
				}
				this->StringCat(str, LVString::Create(_ss(this), _LC("(empty)\n")), str);
			}

			for (int i = 0; i < ident; ++i) {
				this->StringCat(str, LVString::Create(_ss(this), _LC("  ")), str);
			}
			this->StringCat(str, LVString::Create(_ss(this), _LC("}")), str);
			res = str;
			return true;
		}
		case OT_ARRAY: {
			LVObjectPtr refidx, key, val, rs, str = LVString::Create(_ss(this), _LC("(array) [\n"));
			LVInteger idx;

			/* TODO: optimize */
			while ((idx = _array(o)->Next(refidx, key, val)) != -1) {
				refidx = idx;
				this->ToString(val, rs, ident + 1);
				for (int i = 0; i < ident + 1; ++i) {
					this->StringCat(str, LVString::Create(_ss(this), _LC("  ")), str);
				}
				this->StringCat(str, LVString::Create(_ss(this), _LC("[")), str);
				this->StringCat(str, key, str);
				this->StringCat(str, LVString::Create(_ss(this), _LC("] = ")), str);
				this->StringCat(str, rs, str);
				this->StringCat(str, LVString::Create(_ss(this), _LC(",\n")), str);
			}

			if (!_array(o)->Size()) {
				for (int i = 0; i < ident + 1; ++i) {
					this->StringCat(str, LVString::Create(_ss(this), _LC("  ")), str);
				}
				this->StringCat(str, LVString::Create(_ss(this), _LC("(empty)\n")), str);
			}

			for (int i = 0; i < ident; ++i) {
				this->StringCat(str, LVString::Create(_ss(this), _LC("  ")), str);
			}
			this->StringCat(str, LVString::Create(_ss(this), _LC("]")), str);
			res = str;
			return true;
		}
		case OT_WEAKREF: {
			LVObjectPtr rs, str;
			str = LVString::Create(_ss(this), _LC("(weak reference) -> "));
			this->ToString(_weakref(o)->_obj, rs);
			this->StringCat(str, rs, str);
			res = str;
			return true;
		}
		case OT_USERDATA:
		case OT_INSTANCE:
			if (_delegable(o)->_delegate) {
				LVObjectPtr closure;
				if (_delegable(o)->GetMetaMethod(this, MT_TOSTRING, closure)) {
					Push(o);
					if (CallMetaMethod(closure, MT_TOSTRING, 1, res)) {
						if (type(res) == OT_STRING)
							return true;
					} else {
						return false;
					}
				}
			}
		default:
			scsprintf(_sp(lv_rsl((sizeof(void *) * 2) + NUMBER_MAX_CHAR)), lv_rsl((sizeof(void *) * 2) + NUMBER_MAX_CHAR), _LC("(%s)"), GetTypeName(o));
	}
	res = LVString::Create(_ss(this), _spval);
	return true;
}

bool LVVM::StringCat(const LVObjectPtr& str, const LVObjectPtr& obj, LVObjectPtr& dest) {
	LVObjectPtr a, b;
	if (!ToString(str, a))
		return false;
	if (!ToString(obj, b))
		return false;
	LVInteger l = _string(a)->_len;
	LVInteger ol = _string(b)->_len;
	LVChar *s = _sp(lv_rsl(l + ol + 1));
	memcpy(s, _stringval(a), lv_rsl(l));
	memcpy(s + l, _stringval(b), lv_rsl(ol));
	dest = LVString::Create(_ss(this), _spval, l + ol);
	return true;
}

bool LVVM::TypeOf(const LVObjectPtr& obj1, LVObjectPtr& dest) {
	if (is_delegable(obj1) && _delegable(obj1)->_delegate) {
		LVObjectPtr closure;
		if (_delegable(obj1)->GetMetaMethod(this, MT_TYPEOF, closure)) {
			Push(obj1);
			return CallMetaMethod(closure, MT_TYPEOF, 1, dest);
		}
	}
	dest = LVString::Create(_ss(this), GetTypeName(obj1));
	return true;
}

bool LVVM::Init(LVVM *friendvm, LVInteger stacksize) {
	_stack.resize(stacksize);
	_alloccallsstacksize = 4;
	_callstackdata.resize(_alloccallsstacksize);
	_callsstacksize = 0;
	_callsstack = &_callstackdata[0];
	_stackbase = 0;
	_top = 0;
	if (!friendvm) {
		_roottable = LVTable::Create(_ss(this), 0);
		lv_base_register(this);
	} else {
		_roottable = friendvm->_roottable;
		_errorhandler = friendvm->_errorhandler;
		_debughook = friendvm->_debughook;
		_debughook_native = friendvm->_debughook_native;
		_debughook_closure = friendvm->_debughook_closure;
	}

	return true;
}

/* Starts a call in the same "Execution loop" */
bool LVVM::StartCall(LVClosure *closure, LVInteger target, LVInteger args, LVInteger stackbase, bool tailcall) {
	FunctionPrototype *func = closure->_function;
	LVInteger paramssize = func->_nparameters;
	const LVInteger newtop = stackbase + func->_stacksize;
	LVInteger nargs = args;

	if (func->_varparams) {
		paramssize--;
		if (nargs < paramssize) {
			Raise_Error(_LC("wrong number of parameters"));
			return false;
		}

		// dumpstack(stackbase);
		LVInteger nvargs = nargs - paramssize;
		LVArray *arr = LVArray::Create(_ss(this), nvargs);
		LVInteger pbase = stackbase + paramssize;
		for (LVInteger n = 0; n < nvargs; n++) {
			arr->_values[n] = _stack._vals[pbase];
			_stack._vals[pbase].Null();
			pbase++;

		}
		_stack._vals[stackbase + paramssize] = arr;
		//dumpstack(stackbase);
	} else if (paramssize != nargs) {
		LVInteger ndef = func->_ndefaultparams;
		LVInteger diff;
		if (ndef && nargs < paramssize && (diff = paramssize - nargs) <= ndef) {
			for (LVInteger n = ndef - diff; n < ndef; n++) {
				_stack._vals[stackbase + (nargs++)] = closure->_defaultparams[n];
			}
		} else {
			Raise_Error(_LC("wrong number of parameters"));
			return false;
		}
	}

	if (closure->_env) {
		_stack._vals[stackbase] = closure->_env->_obj;
	}

	if (!EnterFrame(stackbase, newtop, tailcall))
		return false;

	ci->_closure  = closure;
	ci->_literals = func->_literals;
	ci->_ip       = func->_instructions;
	ci->_target   = (LVInt32)target;

	if (_debughook) {
		CallDebugHook(_LC('c'));
	}

	if (closure->_function->_bgenerator) {
		FunctionPrototype *f = closure->_function;
		LVGenerator *gen = LVGenerator::Create(_ss(this), closure);
		if (!gen->Yield(this, f->_stacksize))
			return false;
		LVObjectPtr temp;
		Return(1, target, temp);
		STK(target) = gen;
	}

	return true;
}

bool LVVM::Return(LVInteger _arg0, LVInteger _arg1, LVObjectPtr& retval) {
	LVBool    _isroot      = ci->_root;
	LVInteger callerbase   = _stackbase - ci->_prevstkbase;

	if (_debughook) {
		for (LVInteger i = 0; i < ci->_ncalls; i++) {
			CallDebugHook(_LC('r'));
		}
	}

	LVObjectPtr *dest;
	if (_isroot) {
		dest = &(retval);
	} else if (ci->_target == -1) {
		dest = NULL;
	} else {
		dest = &_stack._vals[callerbase + ci->_target];
	}
	if (dest) {
		if (_arg0 != 0xFF) {
			*dest = _stack._vals[_stackbase + _arg1];
		} else {
			dest->Null();
		}
		//*dest = (_arg0 != 0xFF) ? _stack._vals[_stackbase+_arg1] : _null_;
	}
	LeaveFrame();
	return _isroot ? true : false;
}

#define _RET_ON_FAIL(exp) { if(!exp) return false; }

bool LVVM::PLOCAL_INC(LVInteger op, LVObjectPtr& target, LVObjectPtr& a, LVObjectPtr& incr) {
	LVObjectPtr trg;
	_RET_ON_FAIL(ARITH_OP( op , trg, a, incr));
	target = a;
	a = trg;
	return true;
}

bool LVVM::DerefInc(LVInteger op, LVObjectPtr& target, LVObjectPtr& self, LVObjectPtr& key, LVObjectPtr& incr, bool postfix, LVInteger selfidx) {
	LVObjectPtr tmp, tself = self, tkey = key;
	if (!Get(tself, tkey, tmp, 0, selfidx)) {
		return false;
	}
	_RET_ON_FAIL(ARITH_OP( op , target, tmp, incr))
	if (!Set(tself, tkey, target, selfidx)) {
		return false;
	}
	if (postfix) target = tmp;
	return true;
}

#define arg0 (_i_._arg0)
#define sarg0 ((LVInteger)*((const signed char *)&_i_._arg0))
#define arg1 (_i_._arg1)
#define sarg1 (*((const LVInt32 *)&_i_._arg1))
#define arg2 (_i_._arg2)
#define arg3 (_i_._arg3)
#define sarg3 ((LVInteger)*((const signed char *)&_i_._arg3))

LVRESULT LVVM::Suspend() {
	if (_suspended)
		return lv_throwerror(this, _LC("cannot suspend an already suspended vm"));
	if (_nnativecalls != 2)
		return lv_throwerror(this, _LC("cannot suspend through native calls/metamethods"));
	return SUSPEND_FLAG;
}


#define _FINISH(howmuchtojump) {jump = howmuchtojump; return true; }
bool LVVM::FOREACH_OP(LVObjectPtr& o1, LVObjectPtr& o2, LVObjectPtr
                      &o3, LVObjectPtr& o4, LVInteger LV_UNUSED_ARG(arg_2), int exitpos, int& jump) {
	LVInteger nrefidx;
	switch (type(o1)) {
		case OT_TABLE:
			if ((nrefidx = _table(o1)->Next(false, o4, o2, o3)) == -1) _FINISH(exitpos);
			o4 = (LVInteger)nrefidx;
			_FINISH(1);
		case OT_ARRAY:
			if ((nrefidx = _array(o1)->Next(o4, o2, o3)) == -1) _FINISH(exitpos);
			o4 = (LVInteger) nrefidx;
			_FINISH(1);
		case OT_STRING:
			if ((nrefidx = _string(o1)->Next(o4, o2, o3)) == -1)_FINISH(exitpos);
			o4 = (LVInteger)nrefidx;
			_FINISH(1);
		case OT_CLASS:
			if ((nrefidx = _class(o1)->Next(o4, o2, o3)) == -1)_FINISH(exitpos);
			o4 = (LVInteger)nrefidx;
			_FINISH(1);
		case OT_USERDATA:
		case OT_INSTANCE:
			if (_delegable(o1)->_delegate) {
				LVObjectPtr itr;
				LVObjectPtr closure;
				if (_delegable(o1)->GetMetaMethod(this, MT_NEXTI, closure)) {
					Push(o1);
					Push(o4);
					if (CallMetaMethod(closure, MT_NEXTI, 2, itr)) {
						o4 = o2 = itr;
						if (type(itr) == OT_NULL) _FINISH(exitpos);
						if (!Get(o1, itr, o3, 0, DONT_FALL_BACK)) {
							Raise_Error(_LC("_nexti returned an invalid idx")); // cloud be changed
							return false;
						}
						_FINISH(1);
					} else {
						return false;
					}
				}
				Raise_Error(_LC("_nexti failed"));
				return false;
			}
			break;
		case OT_GENERATOR:
			if (_generator(o1)->_state == LVGenerator::eDead) _FINISH(exitpos);
			if (_generator(o1)->_state == LVGenerator::eSuspended) {
				LVInteger idx = 0;
				if (type(o4) == OT_INTEGER) {
					idx = _integer(o4) + 1;
				}
				o2 = idx;
				o4 = idx;
				_generator(o1)->Resume(this, o3);
				_FINISH(0);
			}
		default:
			Raise_Error(_LC("cannot iterate %s"), GetTypeName(o1));
	}
	return false; //cannot be hit(just to avoid warnings)
}

#define COND_LITERAL (arg3!=0?ci->_literals[arg1]:STK(arg1))

#define THROW() { goto exception_trap; }

#define _GUARD(exp) { if(!exp) { THROW();} }

bool LVVM::CLOSURE_OP(LVObjectPtr& target, FunctionPrototype *func) {
	LVInteger nouters;
	LVClosure *closure = LVClosure::Create(_ss(this), func, _table(_roottable)->GetWeakRef(OT_TABLE));
	if ((nouters = func->_noutervalues)) {
		for (LVInteger i = 0; i < nouters; i++) {
			LVOuterVar& v = func->_outervalues[i];
			switch (v._type) {
				case otLOCAL:
					FindOuter(closure->_outervalues[i], &STK(_integer(v._src)));
					break;
				case otOUTER:
					closure->_outervalues[i] = _closure(ci->_closure)->_outervalues[_integer(v._src)];
					break;
			}
		}
	}
	LVInteger ndefparams;
	if ((ndefparams = func->_ndefaultparams)) {
		for (LVInteger i = 0; i < ndefparams; i++) {
			LVInteger spos = func->_defaultparams[i];
			closure->_defaultparams[i] = _stack._vals[_stackbase + spos];
		}
	}
	target = closure;
	return true;

}

// bool LVVM::CLASS_OP(LVObjectPtr& target, LVInteger baseclass, LVInteger attributes) {
bool LVVM::CLASS_OP(LVObjectPtr& target, LVInteger baseclass, LVInteger abstract) {
	LVClass *base = NULL;
	LVObjectPtr attrs;
	if (baseclass != -1) {
		if (type(_stack._vals[_stackbase + baseclass]) != OT_CLASS) {
			Raise_Error(_LC("trying to inherit from a %s"), GetTypeName(_stack._vals[_stackbase + baseclass]));
			return false;
		}
		base = _class(_stack._vals[_stackbase + baseclass]);
	}
	// if (attributes != MAX_FUNC_STACKSIZE) {
	// 	attrs = _stack._vals[_stackbase + attributes];
	// }
	target = LVClass::Create(_ss(this), base);
	if (type(_class(target)->_metamethods[MT_INHERITED]) != OT_NULL) {
		int nparams = 2;
		LVObjectPtr ret;
		Push(target);
		Push(attrs);
		if (!Call(_class(target)->_metamethods[MT_INHERITED], nparams, _top - nparams, ret, false)) {
			Pop(nparams);
			return false;
		}
		Pop(nparams);
	}
	if (abstract != MAX_FUNC_STACKSIZE) {
		_class(target)->_abstract = true;
	}
	_class(target)->_attributes = attrs;
	return true;
}

bool LVVM::IsEqual(const LVObjectPtr& o1, const LVObjectPtr& o2, bool& res) {
	if (type(o1) == type(o2)) {
		res = (_rawval(o1) == _rawval(o2));
	} else {
		if (lv_isnumeric(o1) && lv_isnumeric(o2)) {
			res = (tofloat(o1) == tofloat(o2));
		} else {
			res = false;
		}
	}
	return true;
}

bool LVVM::IsFalse(LVObjectPtr& o) {
	if (((type(o) & OBJECT_CANBEFALSE)
	        && ( ((type(o) == OT_FLOAT) && (_float(o) == LVFloat(0.0))) ))
#if !defined(USEDOUBLE) || (defined(USEDOUBLE) && defined(_LV64))
	        || (_integer(o) == 0) )  //OT_NULL|OT_INTEGER|OT_BOOL
#else
	        || (((type(o) != OT_FLOAT) && (_integer(o) == 0))) )  //OT_NULL|OT_INTEGER|OT_BOOL
#endif
	{
		return true;
	}
	return false;
}

extern LVInstructionDesc instruction_desc[];
bool LVVM::Execute(LVObjectPtr& closure, LVInteger nargs, LVInteger stackbase, LVObjectPtr& outres, LVBool raiseerror, ExecutionType et) {
	if ((_nnativecalls + 1) > MAX_NATIVE_CALLS) {
		Raise_Error(_LC("Native stack overflow"));
		return false;
	}

	// puts("Execute");

	_nnativecalls++;
	AutoDec ad(&_nnativecalls);
	LVInteger traps = 0;
	CallInfo *prevci = ci;

	switch (et) {
		case ET_CALL: {
			temp_reg = closure;
			if (!StartCall(_closure(temp_reg), _top - nargs, nargs, stackbase, false)) {
				//call the handler if there are no calls in the stack, if not relies on the previous node
				if (ci == NULL)
					CallErrorHandler(_lasterror);
				return false;
			}
			if (ci == prevci) {
				outres = STK(_top - nargs);
				return true;
			}
			ci->_root = LVTrue;
		}
		break;
		case ET_RESUME_GENERATOR:
			_generator(closure)->Resume(this, outres);
			ci->_root = LVTrue;
			traps += ci->_etraps;
			break;
		case ET_RESUME_VM:
		case ET_RESUME_THROW_VM:
			traps = _suspended_traps;
			ci->_root = _suspended_root;
			_suspended = LVFalse;
			if (et  == ET_RESUME_THROW_VM) {
				THROW();
			}
			break;
	}

exception_restore:
	//
	{
		for (;;) {
			const LVInstruction& _i_ = *ci->_ip++;
			// dumpstack(_stackbase);
			// scprintf("\n[%d] %s %d %d %d %d\n", ci->_ip - _closure(ci->_closure)->_function->_instructions, instruction_desc[_i_.op].name, arg0, arg1, arg2, arg3);
			switch (_i_.op) {
				case _OP_LINE:
					if (_debughook)
						CallDebugHook(_LC('l'), arg1);
					continue;
				case _OP_LOAD:
					TARGET = ci->_literals[arg1];
					continue;
				case _OP_LOADINT:
#ifndef _LV64
					TARGET = (LVInteger)arg1;
					continue;
#else
					TARGET = (LVInteger)((LVInt32)arg1);
					continue;
#endif
				case _OP_LOADFLOAT:
					TARGET = *((const LVFloat *)&arg1);
					continue;
				case _OP_DLOAD:
					TARGET = ci->_literals[arg1];
					STK(arg2) = ci->_literals[arg3];
					continue;
				case _OP_TAILCALL: {
					LVObjectPtr& t = STK(arg1);
					if (type(t) == OT_CLOSURE
					        && (!_closure(t)->_function->_bgenerator)) {
						LVObjectPtr clo = t;
						if (_openouters) CloseOuters(&(_stack._vals[_stackbase]));
						for (LVInteger i = 0; i < arg3; i++) STK(i) = STK(arg2 + i);
						_GUARD(StartCall(_closure(clo), ci->_target, arg3, _stackbase, true));
						continue;
					}
				}
				case _OP_CALL: {
					LVObjectPtr clo = STK(arg1);
					switch (type(clo)) {
						case OT_CLOSURE:
							_GUARD(StartCall(_closure(clo), sarg0, arg3, _stackbase + arg2, false));
							continue;
						case OT_NATIVECLOSURE: {
							bool suspend;
							_GUARD(CallNative(_nativeclosure(clo), arg3, _stackbase + arg2, clo, suspend));
							if (suspend) {
								_suspended = LVTrue;
								_suspended_target = sarg0;
								_suspended_root = ci->_root;
								_suspended_traps = traps;
								outres = clo;
								return true;
							}
							if (sarg0 != -1) {
								STK(arg0) = clo;
							}
						}
						continue;
						case OT_CLASS: {
							LVObjectPtr inst;
							LVClass *_class = _class(clo);
							if (_class->_abstract) {
								Raise_Error(_LC("attempt to call abstract object explicit"));
								THROW();
							}
							_GUARD(CreateClassInstance(_class, inst, clo));
							if (sarg0 != -1) {
								STK(arg0) = inst;
							}
							LVInteger stkbase;
							switch (type(clo)) {
								case OT_CLOSURE:
									stkbase = _stackbase + arg2;
									_stack._vals[stkbase] = inst;
									_GUARD(StartCall(_closure(clo), -1, arg3, stkbase, false));
									break;
								case OT_NATIVECLOSURE:
									bool suspend;
									stkbase = _stackbase + arg2;
									_stack._vals[stkbase] = inst;
									_GUARD(CallNative(_nativeclosure(clo), arg3, stkbase, clo, suspend));
									break;
								default:
									break; //shutup GCC 4.x
							}
						}
						break;
						case OT_TABLE:
						case OT_USERDATA:
						case OT_INSTANCE: {
							LVObjectPtr closure;
							if (_delegable(clo)->_delegate && _delegable(clo)->GetMetaMethod(this, MT_CALL, closure)) {
								Push(clo);
								for (LVInteger i = 0; i < arg3; i++) Push(STK(arg2 + i));
								if (!CallMetaMethod(closure, MT_CALL, arg3 + 1, clo)) THROW();
								if (sarg0 != -1) {
									STK(arg0) = clo;
								}
								break;
							}

							//Raise_Error(_LC("attempt to call '%s'"), GetTypeName(clo));
							//THROW();
						}
						default:
							Raise_Error(_LC("attempt to call '%s'"), GetTypeName(clo));
							THROW();
					}
				}
				continue;
				case _OP_PREPCALL:
				case _OP_PREPCALLK: {
					LVObjectPtr& key = _i_.op == _OP_PREPCALLK ? (ci->_literals)[arg1] : STK(arg1);
					LVObjectPtr& o = STK(arg2);
					if (!Get(o, key, temp_reg, 0, arg2)) {
						THROW();
					}
					STK(arg3) = o;
					_Swap(TARGET, temp_reg); //TARGET = temp_reg;
				}
				continue;
				case _OP_GETK:
					if (!Get(STK(arg2), ci->_literals[arg1], temp_reg, 0, arg2)) {
						THROW();
					}
					_Swap(TARGET, temp_reg); //TARGET = temp_reg;
					continue;
				case _OP_MOVE:
					TARGET = STK(arg1);
					continue;
				case _OP_NEWSLOT:
					_GUARD(NewSlot(STK(arg1), STK(arg2), STK(arg3), false));
					if (arg0 != 0xFF) TARGET = STK(arg3);
					continue;
				case _OP_DELETE:
					_GUARD(DeleteSlot(STK(arg1), STK(arg2), TARGET));
					continue;
				case _OP_SET:
					if (!Set(STK(arg1), STK(arg2), STK(arg3), arg1)) {
						THROW();
					}
					if (arg0 != 0xFF) TARGET = STK(arg3);
					continue;
				case _OP_GET:
					if (!Get(STK(arg1), STK(arg2), temp_reg, 0, arg1)) {
						THROW();
					}
					_Swap(TARGET, temp_reg); //TARGET = temp_reg;
					continue;
				case _OP_EQ: {
					bool res;
					if (!IsEqual(STK(arg2), COND_LITERAL, res)) {
						THROW();
					}
					TARGET = res ? true : false;
				}
				continue;
				case _OP_NE: {
					bool res;
					if (!IsEqual(STK(arg2), COND_LITERAL, res)) {
						THROW();
					}
					TARGET = (!res) ? true : false;
				}
				continue;
				case _OP_ADD:
					_ARITH_(+, TARGET, STK(arg2), STK(arg1));
					continue;
				case _OP_SUB:
					_ARITH_(-, TARGET, STK(arg2), STK(arg1));
					continue;
				case _OP_MUL:
					_ARITH_(*, TARGET, STK(arg2), STK(arg1));
					continue;
				case _OP_DIV:
					_ARITH_NOZERO( /, TARGET, STK(arg2), STK(arg1), _LC("division by zero"));
					continue;
				case _OP_MOD:
					ARITH_OP('%', TARGET, STK(arg2), STK(arg1));
					continue;
				case _OP_BITW:
					_GUARD(BW_OP( arg3, TARGET, STK(arg2), STK(arg1)));
					continue;
				case _OP_RETURN:
					if ((ci)->_generator) {
						(ci)->_generator->Kill();
					}
					if (Return(arg0, arg1, temp_reg)) {
						assert(traps == 0);
						//outres = temp_reg;
						_Swap(outres, temp_reg);
						return true;
					}
					continue;
				case _OP_LOADNULLS: {
					for (LVInt32 n = 0; n < arg1; n++) STK(arg0 + n).Null();
				}
				continue;
				case _OP_LOADROOT:  {
					LVWeakRef *w = _closure(ci->_closure)->_root;
					if (type(w->_obj) != OT_NULL) {
						TARGET = w->_obj;
					} else {
						TARGET = _roottable; //shoud this be like this? or null
					}
				}
				continue;
				case _OP_LOADBOOL:
					TARGET = arg1 ? true : false;
					continue;
				case _OP_DMOVE:
					STK(arg0) = STK(arg1);
					STK(arg2) = STK(arg3);
					continue;
				case _OP_JMP:
					ci->_ip += (sarg1);
					continue;
				//case _OP_JNZ: if(!IsFalse(STK(arg0))) ci->_ip+=(sarg1); continue;
				case _OP_JCMP:
					_GUARD(CMP_OP((CmpOP)arg3, STK(arg2), STK(arg0), temp_reg));
					if (IsFalse(temp_reg)) ci->_ip += (sarg1);
					continue;
				case _OP_JZ:
					if (IsFalse(STK(arg0))) ci->_ip += (sarg1);
					continue;
				case _OP_GETOUTER: {
					LVClosure *cur_cls = _closure(ci->_closure);
					LVOuter *otr = _outer(cur_cls->_outervalues[arg1]);
					TARGET = *(otr->_valptr);
				}
				continue;
				case _OP_SETOUTER: {
					LVClosure *cur_cls = _closure(ci->_closure);
					LVOuter   *otr = _outer(cur_cls->_outervalues[arg1]);
					*(otr->_valptr) = STK(arg2);
					if (arg0 != 0xFF) {
						TARGET = STK(arg2);
					}
				}
				continue;
				case _OP_NEWOBJ:
					switch (arg3) {
						case NOT_TABLE:
							TARGET = LVTable::Create(_ss(this), arg1);
							continue;
						case NOT_ARRAY:
							TARGET = LVArray::Create(_ss(this), 0);
							_array(TARGET)->Reserve(arg1);
							continue;
						case NOT_CLASS:
							_GUARD(CLASS_OP(TARGET, arg1, arg2));
							continue;
						default:
							assert(0);
							continue;
					}
				case _OP_APPENDARRAY: {
					LVObject val;
					val._unVal.raw = 0;
					switch (arg2) {
						case AAT_STACK:
							val = STK(arg1);
							break;
						case AAT_LITERAL:
							val = ci->_literals[arg1];
							break;
						case AAT_INT:
							val._type = OT_INTEGER;
#ifndef _LV64
							val._unVal.nInteger = (LVInteger)arg1;
#else
							val._unVal.nInteger = (LVInteger)((LVInt32)arg1);
#endif
							break;
						case AAT_FLOAT:
							val._type = OT_FLOAT;
							val._unVal.fFloat = *((const LVFloat *)&arg1);
							break;
						case AAT_BOOL:
							val._type = OT_BOOL;
							val._unVal.nInteger = arg1;
							break;
						default:
							val._type = OT_INTEGER;
							assert(0);
							break;

					}
					_array(STK(arg0))->Append(val);
					continue;
				}
				case _OP_COMPARITH: {
					LVInteger selfidx = (((LVUnsignedInteger)arg1 & 0xFFFF0000) >> 16);
					_GUARD(DerefInc(arg3, TARGET, STK(selfidx), STK(arg2), STK(arg1 & 0x0000FFFF), false, selfidx));
				}
				continue;
				case _OP_INC: {
					LVObjectPtr o(sarg3);
					_GUARD(DerefInc('+', TARGET, STK(arg1), STK(arg2), o, false, arg1));
				}
				continue;
				case _OP_INCL: {
					LVObjectPtr& a = STK(arg1);
					if (type(a) == OT_INTEGER) {
						a._unVal.nInteger = _integer(a) + sarg3;
					} else {
						LVObjectPtr o(sarg3); //_GUARD(LOCAL_INC('+',TARGET, STK(arg1), o));
						_ARITH_(+, a, a, o);
					}
				}
				continue;
				case _OP_PINC: {
					LVObjectPtr o(sarg3);
					_GUARD(DerefInc('+', TARGET, STK(arg1), STK(arg2), o, true, arg1));
				}
				continue;
				case _OP_PINCL: {
					LVObjectPtr& a = STK(arg1);
					if (type(a) == OT_INTEGER) {
						TARGET = a;
						a._unVal.nInteger = _integer(a) + sarg3;
					} else {
						LVObjectPtr o(sarg3);
						_GUARD(PLOCAL_INC('+', TARGET, STK(arg1), o));
					}

				}
				continue;
				case _OP_CMP:
					_GUARD(CMP_OP((CmpOP)arg3, STK(arg2), STK(arg1), TARGET))  continue;
				case _OP_EXISTS:
					TARGET = Get(STK(arg1), STK(arg2), temp_reg, GET_FLAG_DO_NOT_RAISE_ERROR | GET_FLAG_RAW, DONT_FALL_BACK) ? true : false;
					continue;
				case _OP_INSTANCEOF:
					if (type(STK(arg1)) != OT_CLASS) {
						Raise_Error(_LC("cannot apply instanceof between a %s and a %s"), GetTypeName(STK(arg1)), GetTypeName(STK(arg2)));
						THROW();
					}
					TARGET = (type(STK(arg2)) == OT_INSTANCE) ? (_instance(STK(arg2))->InstanceOf(_class(STK(arg1))) ? true : false) : false;
					continue;
				case _OP_AND:
					if (IsFalse(STK(arg2))) {
						TARGET = STK(arg2);
						ci->_ip += (sarg1);
					}
					continue;
				case _OP_OR:
					if (!IsFalse(STK(arg2))) {
						TARGET = STK(arg2);
						ci->_ip += (sarg1);
					}
					continue;
				case _OP_NEG:
					_GUARD(NEG_OP(TARGET, STK(arg1)));
					continue;
				case _OP_NOT:
					TARGET = IsFalse(STK(arg1));
					continue;
				case _OP_BWNOT:
					if (type(STK(arg1)) == OT_INTEGER) {
						LVInteger t = _integer(STK(arg1));
						TARGET = LVInteger(~t);
						continue;
					}
					Raise_Error(_LC("attempt to perform a bitwise op on a %s"), GetTypeName(STK(arg1)));
					THROW();
				case _OP_CLOSURE: {
					LVClosure *c = ci->_closure._unVal.pClosure;
					FunctionPrototype *fp = c->_function;
					if (!CLOSURE_OP(TARGET, fp->_functions[arg1]._unVal.pFunctionProto)) {
						THROW();
					}
					continue;
				}
				case _OP_YIELD: {
					if (ci->_generator) {
						if (sarg1 != MAX_FUNC_STACKSIZE) temp_reg = STK(arg1);
						_GUARD(ci->_generator->Yield(this, arg2));
						traps -= ci->_etraps;
						if (sarg1 != MAX_FUNC_STACKSIZE) _Swap(STK(arg1), temp_reg); //STK(arg1) = temp_reg;
					} else {
						Raise_Error(_LC("trying to yield a '%s',only genenerator can be yielded"), GetTypeName(ci->_generator));
						THROW();
					}
					if (Return(arg0, arg1, temp_reg)) {
						assert(traps == 0);
						outres = temp_reg;
						return true;
					}

				}
				continue;
				case _OP_RESUME:
					if (type(STK(arg1)) != OT_GENERATOR) {
						Raise_Error(_LC("trying to resume a '%s',only genenerator can be resumed"), GetTypeName(STK(arg1)));
						THROW();
					}
					_GUARD(_generator(STK(arg1))->Resume(this, TARGET));
					traps += ci->_etraps;
					continue;
				case _OP_FOREACH: {
					int tojump;
					_GUARD(FOREACH_OP(STK(arg0), STK(arg2), STK(arg2 + 1), STK(arg2 + 2), arg2, sarg1, tojump));
					ci->_ip += tojump;
				}
				continue;
				case _OP_POSTFOREACH:
					assert(type(STK(arg0)) == OT_GENERATOR);
					if (_generator(STK(arg0))->_state == LVGenerator::eDead)
						ci->_ip += (sarg1 - 1);
					continue;
				case _OP_CLONE:
					_GUARD(Clone(STK(arg1), TARGET));
					continue;
				case _OP_TYPEOF:
					_GUARD(TypeOf(STK(arg1), TARGET)) continue;
				case _OP_PUSHTRAP: {
					LVInstruction *_iv = _closure(ci->_closure)->_function->_instructions;
					_etraps.push_back(LVExceptionTrap(_top, _stackbase, &_iv[(ci->_ip - _iv) + arg1], arg0));
					traps++;
					ci->_etraps++;
				}
				continue;
				case _OP_POPTRAP: {
					for (LVInteger i = 0; i < arg0; i++) {
						_etraps.pop_back();
						traps--;
						ci->_etraps--;
					}
				}
				continue;
				case _OP_THROW:
					Raise_Error(TARGET);
					THROW();
					continue;
				case _OP_NEWSLOTA:
					_GUARD(NewSlotA(STK(arg1), STK(arg2), STK(arg3), (arg0 & NEW_SLOT_ATTRIBUTES_FLAG) ? STK(arg2 - 1) : LVObjectPtr(), (arg0 & NEW_SLOT_STATIC_FLAG) ? true : false, false));
					continue;
				case _OP_GETBASE: {
					LVClosure *clo = _closure(ci->_closure);
					if (clo->_base) {
						TARGET = clo->_base;
					} else {
						TARGET.Null();
					}
					continue;
				}
				case _OP_CLOSE:
					if (_openouters) CloseOuters(&(STK(arg1)));
					continue;
			}

		}
	}
exception_trap: {
		LVObjectPtr currerror = _lasterror;
		//      dumpstack(_stackbase);
		//      LVInteger n = 0;
		LVInteger last_top = _top;

		if (_ss(this)->_notifyallexceptions || (!traps && raiseerror))
			CallErrorHandler(currerror);

		while (ci) {
			if (ci->_etraps > 0) {
				LVExceptionTrap& et = _etraps.top();
				ci->_ip = et._ip;
				_top = et._stacksize;
				_stackbase = et._stackbase;
				_stack._vals[_stackbase + et._extarget] = currerror;
				_etraps.pop_back();
				traps--;
				ci->_etraps--;
				while (last_top >= _top)
					_stack._vals[last_top--].Null();
				goto exception_restore;
			} else if (_debughook) {
				//notify debugger of a "return"
				//even if it really an exception unwinding the stack
				for (LVInteger i = 0; i < ci->_ncalls; i++) {
					CallDebugHook(_LC('r'));
				}
			}
			if (ci->_generator)
				ci->_generator->Kill();
			bool mustbreak = ci && ci->_root;
			LeaveFrame();
			if (mustbreak)
				break;
		}

		_lasterror = currerror;
		return false;
	}
	assert(0);
}

bool LVVM::CreateClassInstance(LVClass *theclass, LVObjectPtr& inst, LVObjectPtr& constructor) {
	inst = theclass->CreateInstance();
	if (!theclass->GetConstructor(constructor)) {
		constructor.Null();
	}
	return true;
}

void LVVM::CallErrorHandler(LVObjectPtr& error) {
	if (type(_errorhandler) != OT_NULL) {
		LVObjectPtr out;
		Push(_roottable);
		Push(error);
		Call(_errorhandler, 2, _top - 2, out, LVFalse);
		Pop(2);
	}
}

void LVVM::CallDebugHook(LVInteger type, LVInteger forcedline) {
	_debughook = false;
	FunctionPrototype *func = _closure(ci->_closure)->_function;
	if (_debughook_native) {
		const LVChar *src = type(func->_sourcename) == OT_STRING ? _stringval(func->_sourcename) : NULL;
		const LVChar *fname = type(func->_name) == OT_STRING ? _stringval(func->_name) : NULL;
		LVInteger line = forcedline ? forcedline : func->GetLine(ci->_ip);
		_debughook_native(this, type, src, line, fname);
	} else {
		LVObjectPtr temp_reg;
		LVInteger nparams = 5;
		Push(_roottable);
		Push(type);
		Push(func->_sourcename);
		Push(forcedline ? forcedline : func->GetLine(ci->_ip));
		Push(func->_name);
		Call(_debughook_closure, nparams, _top - nparams, temp_reg, LVFalse);
		Pop(nparams);
	}
	_debughook = true;
}

/* Starts a native call return when the NATIVE closure returns */
bool LVVM::CallNative(LVNativeClosure *nclosure, LVInteger nargs, LVInteger newbase, LVObjectPtr& retval, bool& suspend) {
	LVInteger nparamscheck = nclosure->_nparamscheck;
	LVInteger newtop = newbase + nargs + nclosure->_noutervalues;

	if (_nnativecalls + 1 > MAX_NATIVE_CALLS) {
		Raise_Error(_LC("Native stack overflow"));
		return false;
	}

	if (nparamscheck && (((nparamscheck > 0) && (nparamscheck != nargs)) ||
	                     ((nparamscheck < 0) && (nargs < (-nparamscheck))))) {
		Raise_Error(_LC("wrong number of parameters"));
		return false;
	}

	LVInteger tcs;
	LVIntVector& tc = nclosure->_typecheck;
	if ((tcs = tc.size())) {
		for (LVInteger i = 0; i < nargs && i < tcs; i++) {
			if ((tc._vals[i] != -1) && !(type(_stack._vals[newbase + i]) & tc._vals[i])) {
				Raise_ParamTypeError(i, tc._vals[i], type(_stack._vals[newbase + i]));
				return false;
			}
		}
	}

	if (!EnterFrame(newbase, newtop, false))
		return false;
	ci->_closure  = nclosure;

	LVInteger outers = nclosure->_noutervalues;
	for (LVInteger i = 0; i < outers; i++) {
		_stack._vals[newbase + nargs + i] = nclosure->_outervalues[i];
	}
	if (nclosure->_env) {
		_stack._vals[newbase] = nclosure->_env->_obj;
	}

	_nnativecalls++;
	LVInteger ret = (nclosure->_function)(this);
	_nnativecalls--;

	suspend = false;
	if (ret == SUSPEND_FLAG) {
		suspend = true;
	} else if (ret < 0) {
		LeaveFrame();
		Raise_Error(_lasterror);
		return false;
	}
	if (ret) {
		retval = _stack._vals[_top - 1];
	} else {
		retval.Null();
	}
	//retval = ret ? _stack._vals[_top-1] : _null_;
	LeaveFrame();
	return true;
}

#define FALLBACK_OK         0
#define FALLBACK_NO_MATCH   1
#define FALLBACK_ERROR      2

bool LVVM::Get(const LVObjectPtr& self, const LVObjectPtr& key, LVObjectPtr& dest, LVUnsignedInteger getflags, LVInteger selfidx) {
	switch (type(self)) {
		case OT_TABLE:
			if (_table(self)->Get(key, dest))
				return true;
			break;
		case OT_ARRAY:
			if (lv_isnumeric(key)) {
				if (_array(self)->Get(tointeger(key), dest)) {
					return true;
				}
				if ((getflags & GET_FLAG_DO_NOT_RAISE_ERROR) == 0)
					Raise_IdxError(key);
				return false;
			}
			break;
		case OT_INSTANCE:
			if (_instance(self)->Get(key, dest))
				return true;
			break;
		case OT_CLASS:
			if (_class(self)->Get(key, dest))
				return true;
			break;
		case OT_STRING:
			if (lv_isnumeric(key)) {
				LVInteger n = tointeger(key);
				LVInteger len = _string(self)->_len;
				if (n < 0) {
					n += len;
				}
				if (n >= 0 && n < len) {
					dest = LVInteger(_stringval(self)[n]);
					return true;
				}
				if ((getflags & GET_FLAG_DO_NOT_RAISE_ERROR) == 0) Raise_IdxError(key);
				return false;
			}
			break;
		default:
			break; //shut up compiler
	}
	if ((getflags & GET_FLAG_RAW) == 0) {
		switch (FallBackGet(self, key, dest)) {
			case FALLBACK_OK:
				return true; //okie
			case FALLBACK_NO_MATCH:
				break; //keep falling back
			case FALLBACK_ERROR:
				return false; // the metamethod failed
		}
		if (InvokeDefaultDelegate(self, key, dest)) {
			return true;
		}
	}
	//#ifdef ROOT_FALLBACK
	if (selfidx == 0) {
		LVWeakRef *w = _closure(ci->_closure)->_root;
		if (type(w->_obj) != OT_NULL) {
			if (Get(*((const LVObjectPtr *)&w->_obj), key, dest, 0, DONT_FALL_BACK)) return true;
		}

	}
	//#endif
	if ((getflags & GET_FLAG_DO_NOT_RAISE_ERROR) == 0)
		Raise_IdxError(key);
	return false;
}

bool LVVM::InvokeDefaultDelegate(const LVObjectPtr& self, const LVObjectPtr& key, LVObjectPtr& dest) {
	LVTable *ddel = NULL;
	switch (type(self)) {
		case OT_CLASS:
			ddel = _class_ddel;
			break;
		case OT_TABLE:
			ddel = _table_ddel;
			break;
		case OT_ARRAY:
			ddel = _array_ddel;
			break;
		case OT_STRING:
			ddel = _string_ddel;
			break;
		case OT_INSTANCE:
			ddel = _instance_ddel;
			break;
		case OT_INTEGER:
		case OT_FLOAT:
		case OT_BOOL:
			ddel = _number_ddel;
			break;
		case OT_GENERATOR:
			ddel = _generator_ddel;
			break;
		case OT_CLOSURE:
		case OT_NATIVECLOSURE:
			ddel = _closure_ddel;
			break;
		case OT_THREAD:
			ddel = _thread_ddel;
			break;
		case OT_WEAKREF:
			ddel = _weakref_ddel;
			break;
		default:
			return false;
	}
	return  ddel->Get(key, dest);
}

LVInteger LVVM::FallBackGet(const LVObjectPtr& self, const LVObjectPtr& key, LVObjectPtr& dest) {
	switch (type(self)) {
		case OT_TABLE:
		case OT_USERDATA:
			//delegation
			if (_delegable(self)->_delegate) {
				if (Get(LVObjectPtr(_delegable(self)->_delegate), key, dest, 0, DONT_FALL_BACK)) return FALLBACK_OK;
			} else {
				return FALLBACK_NO_MATCH;
			}
		//go through
		case OT_INSTANCE: {
			LVObjectPtr closure;
			if (_delegable(self)->GetMetaMethod(this, MT_GET, closure)) {
				Push(self);
				Push(key);
				_nmetamethodscall++;
				AutoDec ad(&_nmetamethodscall);
				if (Call(closure, 2, _top - 2, dest, LVFalse)) {
					Pop(2);
					return FALLBACK_OK;
				} else {
					Pop(2);
					if (type(_lasterror) != OT_NULL) { //NULL means "clean failure" (not found)
						return FALLBACK_ERROR;
					}
				}
			}
		}
		break;
		default:
			break;//shutup GCC 4.x
	}
	// no metamethod or no fallback type
	return FALLBACK_NO_MATCH;
}

bool LVVM::Set(const LVObjectPtr& self, const LVObjectPtr& key, const LVObjectPtr& val, LVInteger selfidx) {
	switch (type(self)) {
		case OT_TABLE:
			if (_table(self)->Set(key, val)) return true;
			break;
		case OT_INSTANCE:
			if (_instance(self)->Set(key, val)) return true;
			break;
		case OT_ARRAY:
			if (!lv_isnumeric(key)) {
				Raise_Error(_LC("indexing %s with %s"), GetTypeName(self), GetTypeName(key));
				return false;
			}
			if (!_array(self)->Set(tointeger(key), val)) {
				Raise_IdxError(key);
				return false;
			}
			return true;
		default:
			Raise_Error(_LC("trying to set '%s'"), GetTypeName(self));
			return false;
	}

	switch (FallBackSet(self, key, val)) {
		case FALLBACK_OK:
			return true; //okie
		case FALLBACK_NO_MATCH:
			break; //keep falling back
		case FALLBACK_ERROR:
			return false; // the metamethod failed
	}
	if (selfidx == 0) {
		if (_table(_roottable)->Set(key, val))
			return true;
	}
	Raise_IdxError(key);
	return false;
}

LVInteger LVVM::FallBackSet(const LVObjectPtr& self, const LVObjectPtr& key, const LVObjectPtr& val) {
	switch (type(self)) {
		case OT_TABLE:
			if (_table(self)->_delegate) {
				if (Set(_table(self)->_delegate, key, val, DONT_FALL_BACK)) return FALLBACK_OK;
			}
		//keps on going
		case OT_INSTANCE:
		case OT_USERDATA: {
			LVObjectPtr closure;
			LVObjectPtr t;
			if (_delegable(self)->GetMetaMethod(this, MT_SET, closure)) {
				Push(self);
				Push(key);
				Push(val);
				_nmetamethodscall++;
				AutoDec ad(&_nmetamethodscall);
				if (Call(closure, 3, _top - 3, t, LVFalse)) {
					Pop(3);
					return FALLBACK_OK;
				} else {
					if (type(_lasterror) != OT_NULL) { //NULL means "clean failure" (not found)
						//error
						Pop(3);
						return FALLBACK_ERROR;
					}
				}
			}
		}
		break;
		default:
			break;//shutup GCC 4.x
	}
	// no metamethod or no fallback type
	return FALLBACK_NO_MATCH;
}

bool LVVM::Clone(const LVObjectPtr& self, LVObjectPtr& target) {
	LVObjectPtr temp_reg;
	LVObjectPtr newobj;
	switch (type(self)) {
		case OT_TABLE:
			newobj = _table(self)->Clone();
			goto cloned_mt;
		case OT_INSTANCE: {
			newobj = _instance(self)->Clone(_ss(this));
cloned_mt:
			LVObjectPtr closure;
			if (_delegable(newobj)->_delegate && _delegable(newobj)->GetMetaMethod(this, MT_CLONED, closure)) {
				Push(newobj);
				Push(self);
				if (!CallMetaMethod(closure, MT_CLONED, 2, temp_reg))
					return false;
			}
		}
		target = newobj;
		return true;
		case OT_ARRAY:
			target = _array(self)->Clone();
			return true;
		default:
			Raise_Error(_LC("cloning a %s"), GetTypeName(self));
			return false;
	}
}

bool LVVM::NewSlotA(const LVObjectPtr& self, const LVObjectPtr& key, const LVObjectPtr& val, const LVObjectPtr& attrs, bool bstatic, bool raw) {
	if (type(self) != OT_CLASS) {
		Raise_Error(_LC("object must be a class"));
		return false;
	}
	LVClass *c = _class(self);
	if (!raw) {
		LVObjectPtr& mm = c->_metamethods[MT_NEWMEMBER];
		if (type(mm) != OT_NULL ) {
			Push(self);
			Push(key);
			Push(val);
			Push(attrs);
			Push(bstatic);
			return CallMetaMethod(mm, MT_NEWMEMBER, 5, temp_reg);
		}
	}
	if (!NewSlot(self, key, val, bstatic))
		return false;
	if (type(attrs) != OT_NULL) {
		c->SetAttributes(key, attrs);
	}
	return true;
}

bool LVVM::NewSlot(const LVObjectPtr& self, const LVObjectPtr& key, const LVObjectPtr& val, bool bstatic) {
	if (type(key) == OT_NULL) {
		Raise_Error(_LC("null cannot be used as index"));
		return false;
	}
	switch (type(self)) {
		case OT_TABLE: {
			bool rawcall = true;
			if (_table(self)->_delegate) {
				LVObjectPtr res;
				if (!_table(self)->Get(key, res)) {
					LVObjectPtr closure;
					if (_delegable(self)->_delegate && _delegable(self)->GetMetaMethod(this, MT_NEWSLOT, closure)) {
						Push(self);
						Push(key);
						Push(val);
						if (!CallMetaMethod(closure, MT_NEWSLOT, 3, res)) {
							return false;
						}
						rawcall = false;
					} else {
						rawcall = true;
					}
				}
			}
			if (rawcall)
				_table(self)->NewSlot(key, val); //cannot fail

			break;
		}
		case OT_INSTANCE: {
			LVObjectPtr res;
			LVObjectPtr closure;
			if (_delegable(self)->_delegate && _delegable(self)->GetMetaMethod(this, MT_NEWSLOT, closure)) {
				Push(self);
				Push(key);
				Push(val);
				if (!CallMetaMethod(closure, MT_NEWSLOT, 3, res)) {
					return false;
				}
				break;
			}
			Raise_Error(_LC("class instances do not support the new slot operator"));
			return false;
			break;
		}
		case OT_CLASS:
			if (!_class(self)->NewSlot(_ss(this), key, val, bstatic)) {
				if (_class(self)->_locked) {
					Raise_Error(_LC("trying to modify a class that has already been instantiated"));
					return false;
				} else {
					LVObjectPtr oval = PrintObjVal(key);
					Raise_Error(_LC("the property '%s' already exists"), _stringval(oval));
					return false;
				}
			}
			break;
		default:
			Raise_Error(_LC("indexing %s with %s"), GetTypeName(self), GetTypeName(key));
			return false;
			break;
	}
	return true;
}

bool LVVM::DeleteSlot(const LVObjectPtr& self, const LVObjectPtr& key, LVObjectPtr& res) {
	switch (type(self)) {
		case OT_TABLE:
		case OT_INSTANCE:
		case OT_USERDATA: {
			LVObjectPtr t;
			//bool handled = false;
			LVObjectPtr closure;
			if (_delegable(self)->_delegate && _delegable(self)->GetMetaMethod(this, MT_DELSLOT, closure)) {
				Push(self);
				Push(key);
				return CallMetaMethod(closure, MT_DELSLOT, 2, res);
			} else {
				if (type(self) == OT_TABLE) {
					if (_table(self)->Get(key, t)) {
						_table(self)->Remove(key);
					} else {
						Raise_IdxError((const LVObject&)key);
						return false;
					}
				} else {
					Raise_Error(_LC("cannot delete a slot from %s"), GetTypeName(self));
					return false;
				}
			}
			res = t;
		}
		break;
		default:
			Raise_Error(_LC("attempt to delete a slot from a %s"), GetTypeName(self));
			return false;
	}
	return true;
}

/* Call a generic closure user defined or NATIVE */
bool LVVM::Call(LVObjectPtr& closure, LVInteger nparams, LVInteger stackbase, LVObjectPtr& outres, LVBool raiseerror) {
#ifdef _DEBUG
	LVInteger prevstackbase = _stackbase;
#endif

	switch (type(closure)) {
		case OT_CLOSURE:
			return Execute(closure, nparams, stackbase, outres, raiseerror);
		case OT_NATIVECLOSURE: {
			bool suspend;
			return CallNative(_nativeclosure(closure), nparams, stackbase, outres, suspend);
		}
		case OT_CLASS: {
			LVObjectPtr constr;
			LVObjectPtr temp;
			CreateClassInstance(_class(closure), outres, constr);
			LVObjectType ctype = type(constr);
			if (ctype == OT_NATIVECLOSURE || ctype == OT_CLOSURE) {
				_stack[stackbase] = outres;
				return Call(constr, nparams, stackbase, temp, raiseerror);
			}
			return true;
		}
		default:
			return false;
	}

#ifdef _DEBUG
	if (!_suspended) {
		assert(_stackbase == prevstackbase);
	}
#endif

	return true;
}

bool LVVM::CallMetaMethod(LVObjectPtr& closure, LVMetaMethod LV_UNUSED_ARG(mm), LVInteger nparams, LVObjectPtr& outres) {
	//LVObjectPtr closure;

	_nmetamethodscall++;
	if (Call(closure, nparams, _top - nparams, outres, LVFalse)) {
		_nmetamethodscall--;
		Pop(nparams);
		return true;
	}
	_nmetamethodscall--;
	//}
	Pop(nparams);
	return false;
}

void LVVM::FindOuter(LVObjectPtr& target, LVObjectPtr *stackindex) {
	LVOuter **pp = &_openouters;
	LVOuter *p;
	LVOuter *otr;

	while ((p = *pp) != NULL && p->_valptr >= stackindex) {
		if (p->_valptr == stackindex) {
			target = LVObjectPtr(p);
			return;
		}
		pp = &p->_next;
	}
	otr = LVOuter::Create(_ss(this), stackindex);
	otr->_next = *pp;
	otr->_idx  = (stackindex - _stack._vals);
	__ObjAddRef(otr);
	*pp = otr;
	target = LVObjectPtr(otr);
}

bool LVVM::EnterFrame(LVInteger newbase, LVInteger newtop, bool tailcall) {
	if ( !tailcall ) {
		if ( _callsstacksize == _alloccallsstacksize ) {
			GrowCallStack();
		}
		ci = &_callsstack[_callsstacksize++];
		ci->_prevstkbase = (LVInt32)(newbase - _stackbase);
		ci->_prevtop = (LVInt32)(_top - _stackbase);
		ci->_etraps = 0;
		ci->_ncalls = 1;
		ci->_generator = NULL;
		ci->_root = LVFalse;
	} else {
		ci->_ncalls++;
	}

	_stackbase = newbase;
	_top = newtop;
	if (newtop + MIN_STACK_OVERHEAD > (LVInteger)_stack.size()) {
		if (_nmetamethodscall) {
			Raise_Error(_LC("stack overflow, cannot resize stack while in a metamethod"));
			return false;
		}
		_stack.resize(newtop + (MIN_STACK_OVERHEAD << 2));
		RelocateOuters();
	}
	return true;
}

void LVVM::LeaveFrame() {
	LVInteger last_top = _top;
	LVInteger last_stackbase = _stackbase;
	LVInteger css = --_callsstacksize;

	/* First clean out the call stack frame */
	ci->_closure.Null();
	_stackbase -= ci->_prevstkbase;
	_top = _stackbase + ci->_prevtop;
	ci = (css) ? &_callsstack[css - 1] : NULL;

	if (_openouters)
		CloseOuters(&(_stack._vals[last_stackbase]));
	while (last_top >= _top) {
		_stack._vals[last_top--].Null();
	}
}

void LVVM::RelocateOuters() {
	LVOuter *p = _openouters;
	while (p) {
		p->_valptr = _stack._vals + p->_idx;
		p = p->_next;
	}
}

void LVVM::CloseOuters(LVObjectPtr *stackindex) {
	LVOuter *p;
	while ((p = _openouters) != NULL && p->_valptr >= stackindex) {
		p->_value = *(p->_valptr);
		p->_valptr = &p->_value;
		_openouters = p->_next;
		__ObjRelease(p);
	}
}

void LVVM::Remove(LVInteger n) {
	n = (n >= 0) ? n + _stackbase - 1 : _top + n;
	for (LVInteger i = n; i < _top; i++) {
		_stack[i] = _stack[i + 1];
	}
	_stack[_top].Null();
	_top--;
}

void LVVM::Pop() {
	_stack[--_top].Null();
}

void LVVM::Pop(LVInteger n) {
	for (LVInteger i = 0; i < n; i++) {
		_stack[--_top].Null();
	}
}

void LVVM::PushNull() {
	_stack[_top++].Null();
}

void LVVM::Push(const LVObjectPtr& o) {
	_stack[_top++] = o;
}

LVObjectPtr& LVVM::Top() {
	return _stack[_top - 1];
}

LVObjectPtr& LVVM::PopGet() {
	return _stack[--_top];
}

LVObjectPtr& LVVM::GetUp(LVInteger n) {
	return _stack[_top + n];
}

LVObjectPtr& LVVM::GetAt(LVInteger n) {
	return _stack[n];
}

#ifdef _DEBUG
void LVVM::dumpstack(LVInteger stackbase, bool dumpall) {
	if (stackbase == -1)
		stackbase = _stackbase;
	LVInteger size = dumpall ? _stack.size() : _top;
	LVInteger n = 0, r = 1;
	typedef lvpair<LVInteger, LVRawObjectVal> respair;
	lvvector<respair> resources(_top + 1);
	scprintf(_LC("\n>>>>stack dump<<<<\n"));
	scprintf(_LC("stack exhaustion: %lld/%lld\n"), _top, _stack.size());

	LVInteger prevbase = -1;
	if (_callsstacksize > 0) {
		CallInfo& ci = _callsstack[_callsstacksize - 1];
		scprintf(_LC("ip: %p\n"), ci._ip);
		scprintf(_LC("prev stack base: %d\n"), ci._prevstkbase);
		scprintf(_LC("prev top: %d\n"), ci._prevtop);
		prevbase = ci._prevstkbase;
	}

	for (LVInteger i = 0; i < size; ++i) {
		LVObjectPtr& obj = _stack[i];
		if (stackbase == i)
			scprintf(_LC(">"));
		else if (prevbase == i)
			scprintf(_LC("*"));
		else
			scprintf(_LC(" "));

		LVInteger res = 0;
		for (LVUnsignedInteger j = 0; j < resources.size(); ++j) {
			if (resources[j].second == _rawval(obj))
				res = resources[j].first;
		}

		if (!res) {
			resources[r] = respair(r, _rawval(obj));
			res = r++;
		}

		scprintf(_LC("[" _PRINT_INT_FMT "]:#" _PRINT_INT_FMT ": "), n, res);
		switch (type(obj)) {
			case OT_FLOAT:
				scprintf(_LC("float %.3f"), _float(obj));
				break;
			case OT_INTEGER:
				scprintf(_LC("integer " _PRINT_INT_FMT), _integer(obj));
				break;
			case OT_BOOL:
				scprintf(_LC("bool %s"), _integer(obj) ? "true" : "false");
				break;
			case OT_STRING:
				scprintf(_LC("string %s"), _stringval(obj));
				break;
			case OT_NULL:
				scprintf(_LC("null"));
				break;
			case OT_TABLE:
				if (_table(_roottable) == _table(obj))
					scprintf(_LC("table(root)[" _PRINT_INT_FMT "] [%p]"), _table(obj)->CountUsed(), _table(obj)->_delegate);
				else
					scprintf(_LC("table[" _PRINT_INT_FMT "] [%p]"), _table(obj)->CountUsed(), _table(obj)->_delegate);
				break;
			case OT_ARRAY:
				scprintf(_LC("array[" _PRINT_INT_FMT "]"), _array(obj)->Size());
				break;
			case OT_CLOSURE:
				scprintf(_LC("closure(function)"));
				break;
			case OT_NATIVECLOSURE:
				scprintf(_LC("native closure(api)"));
				break;
			case OT_USERDATA:
				scprintf(_LC("userdata [%p]"), _userdata(obj)->_delegate);
				break;
			case OT_GENERATOR:
				scprintf(_LC("generator"));
				break;
			case OT_THREAD:
				scprintf(_LC("thread"));
				break;
			case OT_USERPOINTER:
				scprintf(_LC("userpointer"));
				break;
			case OT_CLASS:
				scprintf(_LC("class"));
				break;
			case OT_INSTANCE:
				scprintf(_LC("instance"));
				break;
			case OT_WEAKREF:
				scprintf(_LC("weak reference"));
				break;
			default:
				scprintf(_LC("invalid object type"));
				break;
		};
		scprintf(_LC("\n"));
		++n;
	}
	resources.resize(0);
}
#endif
