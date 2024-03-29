#include "pcheader.h"
#include "stream.h"

#define SETUP_STREAM(v) \
	LVStream *self = NULL; \
	if(LV_FAILED(lv_getinstanceup(v,1,(LVUserPointer*)&self,(LVUserPointer)STREAM_TYPE_TAG))) \
		return lv_throwerror(v,_LC("invalid type tag")); \
	if(!self || !self->IsValid())  \
		return lv_throwerror(v,_LC("the stream is invalid"));

LVInteger _stream_readblob(VMHANDLE v) {
	SETUP_STREAM(v);
	LVUserPointer data, blobp;
	LVInteger size, res;
	lv_getinteger(v, 2, &size);
	if (size > self->Len()) {
		size = self->Len();
	}
	data = lv_getscratchpad(v, size);
	res = self->Read(data, size);
	if (res <= 0)
		return lv_throwerror(v, _LC("no data left to read"));
	blobp = lv_createblob(v, res);
	memcpy(blobp, data, res);
	return 1;
}

#define SAFE_READN(ptr,len) { \
	if(self->Read(ptr,len) != len) \
		return lv_throwerror(v,_LC("io error")); \
	}

LVInteger _stream_readn(VMHANDLE v) {
	SETUP_STREAM(v);
	LVInteger format;
	lv_getinteger(v, 2, &format);
	switch (format) {
		case 'l': {
			LVInteger i;
			SAFE_READN(&i, sizeof(i));
			lv_pushinteger(v, i);
		}
		break;
		case 'i': {
			LVInt32 i;
			SAFE_READN(&i, sizeof(i));
			lv_pushinteger(v, i);
		}
		break;
		case 's': {
			short s;
			SAFE_READN(&s, sizeof(short));
			lv_pushinteger(v, s);
		}
		break;
		case 'w': {
			unsigned short w;
			SAFE_READN(&w, sizeof(unsigned short));
			lv_pushinteger(v, w);
		}
		break;
		case 'c': {
			char c;
			SAFE_READN(&c, sizeof(char));
			lv_pushinteger(v, c);
		}
		break;
		case 'b': {
			unsigned char c;
			SAFE_READN(&c, sizeof(unsigned char));
			lv_pushinteger(v, c);
		}
		break;
		case 'f': {
			float f;
			SAFE_READN(&f, sizeof(float));
			lv_pushfloat(v, f);
		}
		break;
		case 'd': {
			double d;
			SAFE_READN(&d, sizeof(double));
			lv_pushfloat(v, (LVFloat)d);
		}
		break;
		default:
			return lv_throwerror(v, _LC("invalid format"));
	}
	return 1;
}

LVInteger _stream_writeblob(VMHANDLE v) {
	LVUserPointer data;
	LVInteger size;
	SETUP_STREAM(v);
	if (LV_FAILED(lv_getblob(v, 2, &data)))
		return lv_throwerror(v, _LC("invalid parameter"));
	size = lv_getblobsize(v, 2);
	if (self->Write(data, size) != size)
		return lv_throwerror(v, _LC("io error"));
	lv_pushinteger(v, size);
	return 1;
}

LVInteger _stream_writen(VMHANDLE v) {
	SETUP_STREAM(v);
	LVInteger format, ti;
	LVFloat tf;
	lv_getinteger(v, 3, &format);
	switch (format) {
		case 'l': {
			LVInteger i;
			lv_getinteger(v, 2, &ti);
			i = ti;
			self->Write(&i, sizeof(LVInteger));
		}
		break;
		case 'i': {
			LVInt32 i;
			lv_getinteger(v, 2, &ti);
			i = (LVInt32)ti;
			self->Write(&i, sizeof(LVInt32));
		}
		break;
		case 's': {
			short s;
			lv_getinteger(v, 2, &ti);
			s = (short)ti;
			self->Write(&s, sizeof(short));
		}
		break;
		case 'w': {
			unsigned short w;
			lv_getinteger(v, 2, &ti);
			w = (unsigned short)ti;
			self->Write(&w, sizeof(unsigned short));
		}
		break;
		case 'c': {
			char c;
			lv_getinteger(v, 2, &ti);
			c = (char)ti;
			self->Write(&c, sizeof(char));
		}
		break;
		case 'b': {
			unsigned char b;
			lv_getinteger(v, 2, &ti);
			b = (unsigned char)ti;
			self->Write(&b, sizeof(unsigned char));
		}
		break;
		case 'f': {
			float f;
			lv_getfloat(v, 2, &tf);
			f = (float)tf;
			self->Write(&f, sizeof(float));
		}
		break;
		case 'd': {
			double d;
			lv_getfloat(v, 2, &tf);
			d = tf;
			self->Write(&d, sizeof(double));
		}
		break;
		default:
			return lv_throwerror(v, _LC("invalid format"));
	}
	return 0;
}

LVInteger _stream_seek(VMHANDLE v) {
	SETUP_STREAM(v);
	LVInteger offset, origin = LV_SEEK_SET;
	lv_getinteger(v, 2, &offset);
	if (lv_gettop(v) > 2) {
		LVInteger t;
		lv_getinteger(v, 3, &t);
		switch (t) {
			case 'b':
				origin = LV_SEEK_SET;
				break;
			case 'c':
				origin = LV_SEEK_CUR;
				break;
			case 'e':
				origin = LV_SEEK_END;
				break;
			default:
				return lv_throwerror(v, _LC("invalid origin"));
		}
	}
	lv_pushinteger(v, self->Seek(offset, origin));
	return 1;
}

LVInteger _stream_tell(VMHANDLE v) {
	SETUP_STREAM(v);
	lv_pushinteger(v, self->Tell());
	return 1;
}

LVInteger _stream_len(VMHANDLE v) {
	SETUP_STREAM(v);
	lv_pushinteger(v, self->Len());
	return 1;
}

LVInteger _stream_flush(VMHANDLE v) {
	SETUP_STREAM(v);
	if (!self->Flush())
		lv_pushinteger(v, 1);
	else
		lv_pushnull(v);
	return 1;
}

LVInteger _stream_eos(VMHANDLE v) {
	SETUP_STREAM(v);
	if (self->EOS())
		lv_pushinteger(v, 1);
	else
		lv_pushnull(v);
	return 1;
}

LVInteger _stream__cloned(VMHANDLE v) {
	return lv_throwerror(v, _LC("this object cannot be cloned"));
}

static const LVRegFunction _stream_methods[] = {
	_DECL_STREAM_FUNC(readblob, 2, _LC("xn")),
	_DECL_STREAM_FUNC(readn, 2, _LC("xn")),
	_DECL_STREAM_FUNC(writeblob, -2, _LC("xx")),
	_DECL_STREAM_FUNC(writen, 3, _LC("xnn")),
	_DECL_STREAM_FUNC(seek, -2, _LC("xnn")),
	_DECL_STREAM_FUNC(tell, 1, _LC("x")),
	_DECL_STREAM_FUNC(len, 1, _LC("x")),
	_DECL_STREAM_FUNC(eos, 1, _LC("x")),
	_DECL_STREAM_FUNC(flush, 1, _LC("x")),
	_DECL_STREAM_FUNC(_cloned, 0, NULL),
	{NULL, (LVFUNCTION)0, 0, NULL}
};

void init_streamclass(VMHANDLE v) {
	lv_pushregistrytable(v);
	lv_pushstring(v, _LC("std_stream"), -1);
	if (LV_FAILED(lv_get(v, -2))) {
		lv_pushstring(v, _LC("std_stream"), -1);
		lv_newclass(v, LVFalse);
		lv_settypetag(v, -1, (LVUserPointer)STREAM_TYPE_TAG);
		LVInteger i = 0;
		while (_stream_methods[i].name != 0) {
			const LVRegFunction& f = _stream_methods[i];
			lv_pushstring(v, f.name, -1);
			lv_newclosure(v, f.f, 0);
			lv_setparamscheck(v, f.nparamscheck, f.typemask);
			lv_newslot(v, -3, LVFalse);
			i++;
		}
		lv_newslot(v, -3, LVFalse);
		lv_pushroottable(v);
		lv_pushstring(v, _LC("stream"), -1);
		lv_pushstring(v, _LC("std_stream"), -1);
		lv_get(v, -4);
		lv_newslot(v, -3, LVFalse);
		lv_pop(v, 1);
	} else {
		lv_pop(v, 1); //result
	}
	lv_pop(v, 1);
}

LVRESULT declare_stream(VMHANDLE v, const LVChar *name, LVUserPointer typetag, const LVChar *reg_name, const LVRegFunction *methods, const LVRegFunction *globals) {
	if (lv_gettype(v, -1) != OT_TABLE)
		return lv_throwerror(v, _LC("table expected"));
	LVInteger top = lv_gettop(v);
	//create delegate
	init_streamclass(v);
	lv_pushregistrytable(v);
	lv_pushstring(v, reg_name, -1);
	lv_pushstring(v, _LC("std_stream"), -1);
	if (LV_SUCCEEDED(lv_get(v, -3))) {
		lv_newclass(v, LVTrue);
		lv_settypetag(v, -1, typetag);
		LVInteger i = 0;
		while (methods[i].name != 0) {
			const LVRegFunction& f = methods[i];
			lv_pushstring(v, f.name, -1);
			lv_newclosure(v, f.f, 0);
			lv_setparamscheck(v, f.nparamscheck, f.typemask);
			lv_setnativeclosurename(v, -1, f.name);
			lv_newslot(v, -3, LVFalse);
			i++;
		}
		lv_newslot(v, -3, LVFalse);
		lv_pop(v, 1);

		i = 0;
		while (globals[i].name != 0) {
			const LVRegFunction& f = globals[i];
			lv_pushstring(v, f.name, -1);
			lv_newclosure(v, f.f, 0);
			lv_setparamscheck(v, f.nparamscheck, f.typemask);
			lv_setnativeclosurename(v, -1, f.name);
			lv_newslot(v, -3, LVFalse);
			i++;
		}
		//register the class in the target table
		lv_pushstring(v, name, -1);
		lv_pushregistrytable(v);
		lv_pushstring(v, reg_name, -1);
		lv_get(v, -2);
		lv_remove(v, -2);
		lv_newslot(v, -3, LVFalse);

		lv_settop(v, top);
		return LV_OK;
	}
	lv_settop(v, top);
	return LV_ERROR;
}
