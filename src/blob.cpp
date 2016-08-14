#include "pcheader.h"
#include "stream.h"

#define BLOB_TYPE_TAG (STREAM_TYPE_TAG | 0x00000002)

struct LVBlob : public LVStream {
	LVBlob(LVInteger size) {
		_size = size;
		_allocated = size;
		_buf = (unsigned char *)lv_malloc(size);
		memset(_buf, 0, _size);
		_ptr = 0;
		_owns = true;
	}
	virtual ~LVBlob() {
		lv_free(_buf, _allocated);
	}
	LVInteger Write(void *buffer, LVInteger size) {
		if (!CanAdvance(size)) {
			GrowBufOf(_ptr + size - _size);
		}
		memcpy(&_buf[_ptr], buffer, size);
		_ptr += size;
		return size;
	}
	LVInteger Read(void *buffer, LVInteger size) {
		LVInteger n = size;
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
	bool Resize(LVInteger n) {
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
	bool GrowBufOf(LVInteger n) {
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
	bool CanAdvance(LVInteger n) {
		if (_ptr + n > _size)
			return false;
		return true;
	}
	LVInteger Seek(LVInteger offset, LVInteger origin) {
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
	LVInteger Flush() {
		return 0;
	}
	LVInteger Tell() {
		return _ptr;
	}
	LVInteger Len() {
		return _size;
	}
	LVUserPointer GetBuf() {
		return _buf;
	}

  private:
	LVInteger _size;
	LVInteger _allocated;
	LVInteger _ptr;
	unsigned char *_buf;
	bool _owns;
};

#define SETUP_BLOB(v) \
	LVBlob *self = NULL; \
	{ if(LV_FAILED(lv_getinstanceup(v,1,(LVUserPointer*)&self,(LVUserPointer)BLOB_TYPE_TAG))) \
		return lv_throwerror(v,_LC("invalid type tag"));  } \
	if(!self || !self->IsValid())  \
		return lv_throwerror(v,_LC("the blob is invalid"));


static LVInteger _blob_resize(VMHANDLE v) {
	SETUP_BLOB(v);
	LVInteger size;
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

static LVInteger _blob_swap4(VMHANDLE v) {
	SETUP_BLOB(v);
	LVInteger num = (self->Len() - (self->Len() % 4)) >> 2;
	unsigned int *t = (unsigned int *)self->GetBuf();
	for (LVInteger i = 0; i < num; i++) {
		__swap_dword(&t[i]);
	}
	return 0;
}

static LVInteger _blob_swap2(VMHANDLE v) {
	SETUP_BLOB(v);
	LVInteger num = (self->Len() - (self->Len() % 2)) >> 1;
	unsigned short *t = (unsigned short *)self->GetBuf();
	for (LVInteger i = 0; i < num; i++) {
		__swap_word(&t[i]);
	}
	return 0;
}

static LVInteger _blob__set(VMHANDLE v) {
	SETUP_BLOB(v);
	LVInteger idx, val;
	lv_getinteger(v, 2, &idx);
	lv_getinteger(v, 3, &val);
	if (idx < 0 || idx >= self->Len())
		return lv_throwerror(v, _LC("index out of range"));
	((unsigned char *)self->GetBuf())[idx] = (unsigned char) val;
	lv_push(v, 3);
	return 1;
}

static LVInteger _blob__get(VMHANDLE v) {
	SETUP_BLOB(v);
	LVInteger idx;
	lv_getinteger(v, 2, &idx);
	if (idx < 0 || idx >= self->Len())
		return lv_throwerror(v, _LC("index out of range"));
	lv_pushinteger(v, ((unsigned char *)self->GetBuf())[idx]);
	return 1;
}

static LVInteger _blob__nexti(VMHANDLE v) {
	SETUP_BLOB(v);
	if (lv_gettype(v, 2) == OT_NULL) {
		lv_pushinteger(v, 0);
		return 1;
	}
	LVInteger idx;
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

static LVInteger _blob__typeof(VMHANDLE v) {
	lv_pushstring(v, _LC("blob"), -1);
	return 1;
}

static LVInteger _blob_releasehook(LVUserPointer p, LVInteger LV_UNUSED_ARG(size)) {
	LVBlob *self = (LVBlob *)p;
	self->~LVBlob();
	lv_free(self, sizeof(LVBlob));
	return 1;
}

static LVInteger _blob_constructor(VMHANDLE v) {
	LVInteger nparam = lv_gettop(v);
	LVInteger size = 0;
	if (nparam == 2) {
		lv_getinteger(v, 2, &size);
	}
	if (size < 0) return lv_throwerror(v, _LC("cannot create blob with negative size"));
	//LVBlob *b = new LVBlob(size);

	LVBlob *b = new(lv_malloc(sizeof(LVBlob))) LVBlob(size);
	if (LV_FAILED(lv_setinstanceup(v, 1, b))) {
		b->~LVBlob();
		lv_free(b, sizeof(LVBlob));
		return lv_throwerror(v, _LC("cannot create blob"));
	}
	lv_setreleasehook(v, 1, _blob_releasehook);
	return 0;
}

static LVInteger _blob__cloned(VMHANDLE v) {
	LVBlob *other = NULL;
	{
		if (LV_FAILED(lv_getinstanceup(v, 2, (LVUserPointer *)&other, (LVUserPointer)BLOB_TYPE_TAG)))
			return LV_ERROR;
	}
	//LVBlob *thisone = new LVBlob(other->Len());
	LVBlob *thisone = new(lv_malloc(sizeof(LVBlob))) LVBlob(other->Len());
	memcpy(thisone->GetBuf(), other->GetBuf(), thisone->Len());
	if (LV_FAILED(lv_setinstanceup(v, 1, thisone))) {
		thisone->~LVBlob();
		lv_free(thisone, sizeof(LVBlob));
		return lv_throwerror(v, _LC("cannot clone blob"));
	}
	lv_setreleasehook(v, 1, _blob_releasehook);
	return 0;
}

#define _DECL_BLOB_FUNC(name,nparams,typecheck) {_LC(#name),_blob_##name,nparams,typecheck}
static const LVRegFunction _blob_methods[] = {
	_DECL_BLOB_FUNC(constructor, -1, _LC("xn")),
	_DECL_BLOB_FUNC(resize, 2, _LC("xn")),
	_DECL_BLOB_FUNC(swap2, 1, _LC("x")),
	_DECL_BLOB_FUNC(swap4, 1, _LC("x")),
	_DECL_BLOB_FUNC(_set, 3, _LC("xnn")),
	_DECL_BLOB_FUNC(_get, 2, _LC("xn")),
	_DECL_BLOB_FUNC(_typeof, 1, _LC("x")),
	_DECL_BLOB_FUNC(_nexti, 2, _LC("x")),
	_DECL_BLOB_FUNC(_cloned, 2, _LC("xx")),
	{NULL, (LVFUNCTION)0, 0, NULL}
};

//GLOBAL FUNCTIONS

static LVInteger _g_blob_casti2f(VMHANDLE v) {
	LVInteger i;
	lv_getinteger(v, 2, &i);
	lv_pushfloat(v, *((const LVFloat *)&i));
	return 1;
}

static LVInteger _g_blob_castf2i(VMHANDLE v) {
	LVFloat f;
	lv_getfloat(v, 2, &f);
	lv_pushinteger(v, *((const LVInteger *)&f));
	return 1;
}

static LVInteger _g_blob_swap2(VMHANDLE v) {
	LVInteger i;
	lv_getinteger(v, 2, &i);
	short s = (short)i;
	lv_pushinteger(v, (s << 8) | ((s >> 8) & 0x00FF));
	return 1;
}

static LVInteger _g_blob_swap4(VMHANDLE v) {
	LVInteger i;
	lv_getinteger(v, 2, &i);
	unsigned int t4 = (unsigned int)i;
	__swap_dword(&t4);
	lv_pushinteger(v, (LVInteger)t4);
	return 1;
}

static LVInteger _g_blob_swapfloat(VMHANDLE v) {
	LVFloat f;
	lv_getfloat(v, 2, &f);
	__swap_dword((unsigned int *)&f);
	lv_pushfloat(v, f);
	return 1;
}

#define _DECL_GLOBALBLOB_FUNC(name,nparams,typecheck) {_LC(#name),_g_blob_##name,nparams,typecheck}
static const LVRegFunction bloblib_funcs[] = {
	_DECL_GLOBALBLOB_FUNC(casti2f, 2, _LC(".n")),
	_DECL_GLOBALBLOB_FUNC(castf2i, 2, _LC(".n")),
	_DECL_GLOBALBLOB_FUNC(swap2, 2, _LC(".n")),
	_DECL_GLOBALBLOB_FUNC(swap4, 2, _LC(".n")),
	_DECL_GLOBALBLOB_FUNC(swapfloat, 2, _LC(".n")),
	{NULL, (LVFUNCTION)0, 0, NULL}
};

LVRESULT lv_getblob(VMHANDLE v, LVInteger idx, LVUserPointer *ptr) {
	LVBlob *blob;
	if (LV_FAILED(lv_getinstanceup(v, idx, (LVUserPointer *)&blob, (LVUserPointer)BLOB_TYPE_TAG)))
		return -1;
	*ptr = blob->GetBuf();
	return LV_OK;
}

LVInteger lv_getblobsize(VMHANDLE v, LVInteger idx) {
	LVBlob *blob;
	if (LV_FAILED(lv_getinstanceup(v, idx, (LVUserPointer *)&blob, (LVUserPointer)BLOB_TYPE_TAG)))
		return -1;
	return blob->Len();
}

LVUserPointer lv_createblob(VMHANDLE v, LVInteger size) {
	LVInteger top = lv_gettop(v);
	lv_pushregistrytable(v);
	lv_pushstring(v, _LC("std_blob"), -1);
	if (LV_SUCCEEDED(lv_get(v, -2))) {
		lv_remove(v, -2); //removes the registry
		lv_push(v, 1); // push the this
		lv_pushinteger(v, size); //size
		LVBlob *blob = NULL;
		if (LV_SUCCEEDED(lv_call(v, 2, LVTrue, LVFalse))
		        && LV_SUCCEEDED(lv_getinstanceup(v, -1, (LVUserPointer *)&blob, (LVUserPointer)BLOB_TYPE_TAG))) {
			lv_remove(v, -2);
			return blob->GetBuf();
		}
	}
	lv_settop(v, top);
	return NULL;
}

LVRESULT mod_init_blob(VMHANDLE v) {
	return declare_stream(v, _LC("blob"), (LVUserPointer)BLOB_TYPE_TAG, _LC("std_blob"), _blob_methods, bloblib_funcs);
}
