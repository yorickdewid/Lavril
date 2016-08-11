#include "pcheader.h"
#include "stream.h"

#define SQ_BLOB_TYPE_TAG (SQ_STREAM_TYPE_TAG | 0x00000002)

struct SQBlob : public SQStream {
	SQBlob(SQInteger size) {
		_size = size;
		_allocated = size;
		_buf = (unsigned char *)lv_malloc(size);
		memset(_buf, 0, _size);
		_ptr = 0;
		_owns = true;
	}
	virtual ~SQBlob() {
		lv_free(_buf, _allocated);
	}
	SQInteger Write(void *buffer, SQInteger size) {
		if (!CanAdvance(size)) {
			GrowBufOf(_ptr + size - _size);
		}
		memcpy(&_buf[_ptr], buffer, size);
		_ptr += size;
		return size;
	}
	SQInteger Read(void *buffer, SQInteger size) {
		SQInteger n = size;
		if (!CanAdvance(size)) {
			if ((_size - _ptr) > 0)
				n = _size - _ptr;
			else
				return 0;
		}
		memcpy(buffer, &_buf[_ptr], n);
		_ptr += n;
		return n;
	}
	bool Resize(SQInteger n) {
		if (!_owns)
			return false;
		if (n != _allocated) {
			unsigned char *newbuf = (unsigned char *)lv_malloc(n);
			memset(newbuf, 0, n);
			if (_size > n)
				memcpy(newbuf, _buf, n);
			else
				memcpy(newbuf, _buf, _size);
			lv_free(_buf, _allocated);
			_buf = newbuf;
			_allocated = n;
			if (_size > _allocated)
				_size = _allocated;
			if (_ptr > _allocated)
				_ptr = _allocated;
		}
		return true;
	}
	bool GrowBufOf(SQInteger n) {
		bool ret = true;
		if (_size + n > _allocated) {
			if (_size + n > _size * 2)
				ret = Resize(_size + n);
			else
				ret = Resize(_size * 2);
		}
		_size = _size + n;
		return ret;
	}
	bool CanAdvance(SQInteger n) {
		if (_ptr + n > _size)
			return false;
		return true;
	}
	SQInteger Seek(SQInteger offset, SQInteger origin) {
		switch (origin) {
			case LV_SEEK_SET:
				if (offset > _size || offset < 0) return -1;
				_ptr = offset;
				break;
			case LV_SEEK_CUR:
				if (_ptr + offset > _size || _ptr + offset < 0) return -1;
				_ptr += offset;
				break;
			case LV_SEEK_END:
				if (_size + offset > _size || _size + offset < 0) return -1;
				_ptr = _size + offset;
				break;
			default:
				return -1;
		}
		return 0;
	}
	bool IsValid() {
		return _buf ? true : false;
	}
	bool EOS() {
		return _ptr == _size;
	}
	SQInteger Flush() {
		return 0;
	}
	SQInteger Tell() {
		return _ptr;
	}
	SQInteger Len() {
		return _size;
	}
	SQUserPointer GetBuf() {
		return _buf;
	}

  private:
	SQInteger _size;
	SQInteger _allocated;
	SQInteger _ptr;
	unsigned char *_buf;
	bool _owns;
};

#define SETUP_BLOB(v) \
	SQBlob *self = NULL; \
	{ if(LV_FAILED(lv_getinstanceup(v,1,(SQUserPointer*)&self,(SQUserPointer)SQ_BLOB_TYPE_TAG))) \
		return lv_throwerror(v,_LC("invalid type tag"));  } \
	if(!self || !self->IsValid())  \
		return lv_throwerror(v,_LC("the blob is invalid"));


static SQInteger _blob_resize(VMHANDLE v) {
	SETUP_BLOB(v);
	SQInteger size;
	lv_getinteger(v, 2, &size);
	if (!self->Resize(size))
		return lv_throwerror(v, _LC("resize failed"));
	return 0;
}

static void __swap_dword(unsigned int *n) {
	*n = (unsigned int)(((*n & 0xFF000000) >> 24)  |
	                    ((*n & 0x00FF0000) >> 8)  |
	                    ((*n & 0x0000FF00) << 8)  |
	                    ((*n & 0x000000FF) << 24));
}

static void __swap_word(unsigned short *n) {
	*n = (unsigned short)((*n >> 8) & 0x00FF) | ((*n << 8) & 0xFF00);
}

static SQInteger _blob_swap4(VMHANDLE v) {
	SETUP_BLOB(v);
	SQInteger num = (self->Len() - (self->Len() % 4)) >> 2;
	unsigned int *t = (unsigned int *)self->GetBuf();
	for (SQInteger i = 0; i < num; i++) {
		__swap_dword(&t[i]);
	}
	return 0;
}

static SQInteger _blob_swap2(VMHANDLE v) {
	SETUP_BLOB(v);
	SQInteger num = (self->Len() - (self->Len() % 2)) >> 1;
	unsigned short *t = (unsigned short *)self->GetBuf();
	for (SQInteger i = 0; i < num; i++) {
		__swap_word(&t[i]);
	}
	return 0;
}

static SQInteger _blob__set(VMHANDLE v) {
	SETUP_BLOB(v);
	SQInteger idx, val;
	lv_getinteger(v, 2, &idx);
	lv_getinteger(v, 3, &val);
	if (idx < 0 || idx >= self->Len())
		return lv_throwerror(v, _LC("index out of range"));
	((unsigned char *)self->GetBuf())[idx] = (unsigned char) val;
	lv_push(v, 3);
	return 1;
}

static SQInteger _blob__get(VMHANDLE v) {
	SETUP_BLOB(v);
	SQInteger idx;
	lv_getinteger(v, 2, &idx);
	if (idx < 0 || idx >= self->Len())
		return lv_throwerror(v, _LC("index out of range"));
	lv_pushinteger(v, ((unsigned char *)self->GetBuf())[idx]);
	return 1;
}

static SQInteger _blob__nexti(VMHANDLE v) {
	SETUP_BLOB(v);
	if (lv_gettype(v, 2) == OT_NULL) {
		lv_pushinteger(v, 0);
		return 1;
	}
	SQInteger idx;
	if (LV_SUCCEEDED(lv_getinteger(v, 2, &idx))) {
		if (idx + 1 < self->Len()) {
			lv_pushinteger(v, idx + 1);
			return 1;
		}
		lv_pushnull(v);
		return 1;
	}
	return lv_throwerror(v, _LC("internal error (_nexti) wrong argument type"));
}

static SQInteger _blob__typeof(VMHANDLE v) {
	lv_pushstring(v, _LC("blob"), -1);
	return 1;
}

static SQInteger _blob_releasehook(SQUserPointer p, SQInteger LV_UNUSED_ARG(size)) {
	SQBlob *self = (SQBlob *)p;
	self->~SQBlob();
	lv_free(self, sizeof(SQBlob));
	return 1;
}

static SQInteger _blob_constructor(VMHANDLE v) {
	SQInteger nparam = lv_gettop(v);
	SQInteger size = 0;
	if (nparam == 2) {
		lv_getinteger(v, 2, &size);
	}
	if (size < 0) return lv_throwerror(v, _LC("cannot create blob with negative size"));
	//SQBlob *b = new SQBlob(size);

	SQBlob *b = new(lv_malloc(sizeof(SQBlob))) SQBlob(size);
	if (LV_FAILED(lv_setinstanceup(v, 1, b))) {
		b->~SQBlob();
		lv_free(b, sizeof(SQBlob));
		return lv_throwerror(v, _LC("cannot create blob"));
	}
	lv_setreleasehook(v, 1, _blob_releasehook);
	return 0;
}

static SQInteger _blob__cloned(VMHANDLE v) {
	SQBlob *other = NULL;
	{
		if (LV_FAILED(lv_getinstanceup(v, 2, (SQUserPointer *)&other, (SQUserPointer)SQ_BLOB_TYPE_TAG)))
			return LV_ERROR;
	}
	//SQBlob *thisone = new SQBlob(other->Len());
	SQBlob *thisone = new(lv_malloc(sizeof(SQBlob))) SQBlob(other->Len());
	memcpy(thisone->GetBuf(), other->GetBuf(), thisone->Len());
	if (LV_FAILED(lv_setinstanceup(v, 1, thisone))) {
		thisone->~SQBlob();
		lv_free(thisone, sizeof(SQBlob));
		return lv_throwerror(v, _LC("cannot clone blob"));
	}
	lv_setreleasehook(v, 1, _blob_releasehook);
	return 0;
}

#define _DECL_BLOB_FUNC(name,nparams,typecheck) {_LC(#name),_blob_##name,nparams,typecheck}
static const SQRegFunction _blob_methods[] = {
	_DECL_BLOB_FUNC(constructor, -1, _LC("xn")),
	_DECL_BLOB_FUNC(resize, 2, _LC("xn")),
	_DECL_BLOB_FUNC(swap2, 1, _LC("x")),
	_DECL_BLOB_FUNC(swap4, 1, _LC("x")),
	_DECL_BLOB_FUNC(_set, 3, _LC("xnn")),
	_DECL_BLOB_FUNC(_get, 2, _LC("xn")),
	_DECL_BLOB_FUNC(_typeof, 1, _LC("x")),
	_DECL_BLOB_FUNC(_nexti, 2, _LC("x")),
	_DECL_BLOB_FUNC(_cloned, 2, _LC("xx")),
	{NULL, (SQFUNCTION)0, 0, NULL}
};

//GLOBAL FUNCTIONS

static SQInteger _g_blob_casti2f(VMHANDLE v) {
	SQInteger i;
	lv_getinteger(v, 2, &i);
	lv_pushfloat(v, *((const SQFloat *)&i));
	return 1;
}

static SQInteger _g_blob_castf2i(VMHANDLE v) {
	SQFloat f;
	lv_getfloat(v, 2, &f);
	lv_pushinteger(v, *((const SQInteger *)&f));
	return 1;
}

static SQInteger _g_blob_swap2(VMHANDLE v) {
	SQInteger i;
	lv_getinteger(v, 2, &i);
	short s = (short)i;
	lv_pushinteger(v, (s << 8) | ((s >> 8) & 0x00FF));
	return 1;
}

static SQInteger _g_blob_swap4(VMHANDLE v) {
	SQInteger i;
	lv_getinteger(v, 2, &i);
	unsigned int t4 = (unsigned int)i;
	__swap_dword(&t4);
	lv_pushinteger(v, (SQInteger)t4);
	return 1;
}

static SQInteger _g_blob_swapfloat(VMHANDLE v) {
	SQFloat f;
	lv_getfloat(v, 2, &f);
	__swap_dword((unsigned int *)&f);
	lv_pushfloat(v, f);
	return 1;
}

#define _DECL_GLOBALBLOB_FUNC(name,nparams,typecheck) {_LC(#name),_g_blob_##name,nparams,typecheck}
static const SQRegFunction bloblib_funcs[] = {
	_DECL_GLOBALBLOB_FUNC(casti2f, 2, _LC(".n")),
	_DECL_GLOBALBLOB_FUNC(castf2i, 2, _LC(".n")),
	_DECL_GLOBALBLOB_FUNC(swap2, 2, _LC(".n")),
	_DECL_GLOBALBLOB_FUNC(swap4, 2, _LC(".n")),
	_DECL_GLOBALBLOB_FUNC(swapfloat, 2, _LC(".n")),
	{NULL, (SQFUNCTION)0, 0, NULL}
};

SQRESULT lv_getblob(VMHANDLE v, SQInteger idx, SQUserPointer *ptr) {
	SQBlob *blob;
	if (LV_FAILED(lv_getinstanceup(v, idx, (SQUserPointer *)&blob, (SQUserPointer)SQ_BLOB_TYPE_TAG)))
		return -1;
	*ptr = blob->GetBuf();
	return LV_OK;
}

SQInteger lv_getblobsize(VMHANDLE v, SQInteger idx) {
	SQBlob *blob;
	if (LV_FAILED(lv_getinstanceup(v, idx, (SQUserPointer *)&blob, (SQUserPointer)SQ_BLOB_TYPE_TAG)))
		return -1;
	return blob->Len();
}

SQUserPointer lv_createblob(VMHANDLE v, SQInteger size) {
	SQInteger top = lv_gettop(v);
	lv_pushregistrytable(v);
	lv_pushstring(v, _LC("std_blob"), -1);
	if (LV_SUCCEEDED(lv_get(v, -2))) {
		lv_remove(v, -2); //removes the registry
		lv_push(v, 1); // push the this
		lv_pushinteger(v, size); //size
		SQBlob *blob = NULL;
		if (LV_SUCCEEDED(lv_call(v, 2, LVTrue, LVFalse))
		        && LV_SUCCEEDED(lv_getinstanceup(v, -1, (SQUserPointer *)&blob, (SQUserPointer)SQ_BLOB_TYPE_TAG))) {
			lv_remove(v, -2);
			return blob->GetBuf();
		}
	}
	lv_settop(v, top);
	return NULL;
}

SQRESULT mod_init_blob(VMHANDLE v) {
	return declare_stream(v, _LC("blob"), (SQUserPointer)SQ_BLOB_TYPE_TAG, _LC("std_blob"), _blob_methods, bloblib_funcs);
}
