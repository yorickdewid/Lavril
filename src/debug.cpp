#include "pcheader.h"
#include <stdarg.h>
#include "vm.h"
#include "funcproto.h"
#include "closure.h"
#include "lvstring.h"

LVRESULT lv_getfunctioninfo(VMHANDLE v, LVInteger level, SQFunctionInfo *fi) {
	LVInteger cssize = v->_callsstacksize;
	if (cssize > level) {
		SQVM::CallInfo& ci = v->_callsstack[cssize - level - 1];
		if (lv_isclosure(ci._closure)) {
			SQClosure *c = _closure(ci._closure);
			FunctionPrototype *proto = c->_function;
			fi->funcid = proto;
			fi->name = type(proto->_name) == OT_STRING ? _stringval(proto->_name) : _LC("unknown");
			fi->source = type(proto->_sourcename) == OT_STRING ? _stringval(proto->_sourcename) : _LC("unknown");
			fi->line = proto->_lineinfos[0]._line;
			return LV_OK;
		}
	}
	return lv_throwerror(v, _LC("the object is not a closure"));
}

LVRESULT lv_stackinfos(VMHANDLE v, LVInteger level, SQStackInfos *si) {
	LVInteger cssize = v->_callsstacksize;
	if (cssize > level) {
		memset(si, 0, sizeof(SQStackInfos));
		SQVM::CallInfo& ci = v->_callsstack[cssize - level - 1];
		switch (type(ci._closure)) {
			case OT_CLOSURE: {
				FunctionPrototype *func = _closure(ci._closure)->_function;
				if (type(func->_name) == OT_STRING)
					si->funcname = _stringval(func->_name);
				if (type(func->_sourcename) == OT_STRING)
					si->source = _stringval(func->_sourcename);
				si->line = func->GetLine(ci._ip);
			}
			break;
			case OT_NATIVECLOSURE:
				si->source = _LC("NATIVE");
				si->funcname = _LC("unknown");
				if (type(_nativeclosure(ci._closure)->_name) == OT_STRING)
					si->funcname = _stringval(_nativeclosure(ci._closure)->_name);
				si->line = -1;
				break;
			default:
				break; //shutup compiler
		}
		return LV_OK;
	}
	return LV_ERROR;
}

void SQVM::Raise_Error(const LVChar *s, ...) {
	va_list vl;
	va_start(vl, s);
	LVInteger buffersize = (LVInteger)scstrlen(s) + (NUMBER_MAX_CHAR * 2);
	scvsprintf(_sp(lv_rsl(buffersize)), buffersize, s, vl);
	va_end(vl);
	_lasterror = SQString::Create(_ss(this), _spval, -1);
}

void SQVM::Raise_Error(const SQObjectPtr& desc) {
	_lasterror = desc;
}

SQString *SQVM::PrintObjVal(const SQObjectPtr& o) {
	switch (type(o)) {
		case OT_STRING:
			return _string(o);
		case OT_INTEGER:
			scsprintf(_sp(lv_rsl(NUMBER_MAX_CHAR + 1)), lv_rsl(NUMBER_MAX_CHAR), _PRINT_INT_FMT, _integer(o));
			return SQString::Create(_ss(this), _spval);
			break;
		case OT_FLOAT:
			scsprintf(_sp(lv_rsl(NUMBER_MAX_CHAR + 1)), lv_rsl(NUMBER_MAX_CHAR), _LC("%.14g"), _float(o));
			return SQString::Create(_ss(this), _spval);
			break;
		default:
			return SQString::Create(_ss(this), GetTypeName(o));
	}
}

void SQVM::Raise_IdxError(const SQObjectPtr& o) {
	SQObjectPtr oval = PrintObjVal(o);
	Raise_Error(_LC("the index '%.50s' does not exist"), _stringval(oval));
}

void SQVM::Raise_CompareError(const SQObject& o1, const SQObject& o2) {
	SQObjectPtr oval1 = PrintObjVal(o1), oval2 = PrintObjVal(o2);
	Raise_Error(_LC("comparison between '%.50s' and '%.50s'"), _stringval(oval1), _stringval(oval2));
}

void SQVM::Raise_ParamTypeError(LVInteger nparam, LVInteger typemask, LVInteger type) {
	SQObjectPtr exptypes = SQString::Create(_ss(this), _LC(""), -1);
	LVInteger found = 0;
	for (LVInteger i = 0; i < 16; i++) {
		LVInteger mask = 0x00000001 << i;
		if (typemask & (mask)) {
			if (found > 0) StringCat(exptypes, SQString::Create(_ss(this), _LC("|"), -1), exptypes);
			found ++;
			StringCat(exptypes, SQString::Create(_ss(this), IdType2Name((SQObjectType)mask), -1), exptypes);
		}
	}
	Raise_Error(_LC("parameter %d has an invalid type '%s' ; expected: '%s'"), nparam, IdType2Name((SQObjectType)type), _stringval(exptypes));
}
