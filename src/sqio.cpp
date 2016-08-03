#include "sqpcheader.h"
#include "sqstream.h"

#define SQ_FILE_TYPE_TAG (SQ_STREAM_TYPE_TAG | 0x00000001)

SQFILE lv_fopen(const SQChar *filename , const SQChar *mode) {
#ifndef SQUNICODE
	return (SQFILE)fopen(filename, mode);
#else
	return (SQFILE)_wfopen(filename, mode);
#endif
}

SQInteger lv_fread(void *buffer, SQInteger size, SQInteger count, SQFILE file) {
	SQInteger ret = (SQInteger)fread(buffer, size, count, (FILE *)file);
	return ret;
}

SQInteger lv_fwrite(const SQUserPointer buffer, SQInteger size, SQInteger count, SQFILE file) {
	return (SQInteger)fwrite(buffer, size, count, (FILE *)file);
}

SQInteger lv_fseek(SQFILE file, SQInteger offset, SQInteger origin) {
	SQInteger realorigin;
	switch (origin) {
		case SQ_SEEK_CUR:
			realorigin = SEEK_CUR;
			break;
		case SQ_SEEK_END:
			realorigin = SEEK_END;
			break;
		case SQ_SEEK_SET:
			realorigin = SEEK_SET;
			break;
		default:
			return -1; //failed
	}
	return fseek((FILE *)file, (long)offset, (int)realorigin);
}

SQInteger lv_ftell(SQFILE file) {
	return ftell((FILE *)file);
}

SQInteger lv_fflush(SQFILE file) {
	return fflush((FILE *)file);
}

SQInteger lv_fclose(SQFILE file) {
	return fclose((FILE *)file);
}

SQInteger lv_feof(SQFILE file) {
	return feof((FILE *)file);
}

//File
struct SQFile : public SQStream {
	SQFile() {
		_handle = NULL;
		_owns = false;
	}
	SQFile(SQFILE file, bool owns) {
		_handle = file;
		_owns = owns;
	}
	virtual ~SQFile() {
		Close();
	}
	bool Open(const SQChar *filename , const SQChar *mode) {
		Close();
		if ( (_handle = lv_fopen(filename, mode)) ) {
			_owns = true;
			return true;
		}
		return false;
	}
	void Close() {
		if (_handle && _owns) {
			lv_fclose(_handle);
			_handle = NULL;
			_owns = false;
		}
	}
	SQInteger Read(void *buffer, SQInteger size) {
		return lv_fread(buffer, 1, size, _handle);
	}
	SQInteger Write(void *buffer, SQInteger size) {
		return lv_fwrite(buffer, 1, size, _handle);
	}
	SQInteger Flush() {
		return lv_fflush(_handle);
	}
	SQInteger Tell() {
		return lv_ftell(_handle);
	}
	SQInteger Len() {
		SQInteger prevpos = Tell();
		Seek(0, SQ_SEEK_END);
		SQInteger size = Tell();
		Seek(prevpos, SQ_SEEK_SET);
		return size;
	}
	SQInteger Seek(SQInteger offset, SQInteger origin)  {
		return lv_fseek(_handle, offset, origin);
	}
	bool IsValid() {
		return _handle ? true : false;
	}
	bool EOS() {
		return Tell() == Len() ? true : false;
	}
	SQFILE GetHandle() {
		return _handle;
	}
  private:
	SQFILE _handle;
	bool _owns;
};

static SQInteger _file__typeof(HSQUIRRELVM v) {
	lv_pushstring(v, _SC("file"), -1);
	return 1;
}

static SQInteger _file_releasehook(SQUserPointer p, SQInteger LV_UNUSED_ARG(size)) {
	SQFile *self = (SQFile *)p;
	self->~SQFile();
	lv_free(self, sizeof(SQFile));
	return 1;
}

static SQInteger _file_constructor(HSQUIRRELVM v) {
	const SQChar *filename, *mode;
	bool owns = true;
	SQFile *f;
	SQFILE newf;
	if (lv_gettype(v, 2) == OT_STRING && lv_gettype(v, 3) == OT_STRING) {
		lv_getstring(v, 2, &filename);
		lv_getstring(v, 3, &mode);
		newf = lv_fopen(filename, mode);
		if (!newf) return lv_throwerror(v, _SC("cannot open file"));
	} else if (lv_gettype(v, 2) == OT_USERPOINTER) {
		owns = !(lv_gettype(v, 3) == OT_NULL);
		lv_getuserpointer(v, 2, &newf);
	} else {
		return lv_throwerror(v, _SC("wrong parameter"));
	}

	f = new(lv_malloc(sizeof(SQFile))) SQFile(newf, owns);
	if (LV_FAILED(lv_setinstanceup(v, 1, f))) {
		f->~SQFile();
		lv_free(f, sizeof(SQFile));
		return lv_throwerror(v, _SC("cannot create blob with negative size"));
	}
	lv_setreleasehook(v, 1, _file_releasehook);
	return 0;
}

static SQInteger _file_close(HSQUIRRELVM v) {
	SQFile *self = NULL;
	if (LV_SUCCEEDED(lv_getinstanceup(v, 1, (SQUserPointer *)&self, (SQUserPointer)SQ_FILE_TYPE_TAG))
	        && self != NULL) {
		self->Close();
	}
	return 0;
}

//bindings
#define _DECL_FILE_FUNC(name,nparams,typecheck) {_SC(#name),_file_##name,nparams,typecheck}
static const SQRegFunction _file_methods[] = {
	_DECL_FILE_FUNC(constructor, 3, _SC("x")),
	_DECL_FILE_FUNC(_typeof, 1, _SC("x")),
	_DECL_FILE_FUNC(close, 1, _SC("x")),
	{NULL, (SQFUNCTION)0, 0, NULL}
};

SQRESULT lv_createfile(HSQUIRRELVM v, SQFILE file, SQBool own) {
	SQInteger top = lv_gettop(v);
	lv_pushregistrytable(v);
	lv_pushstring(v, _SC("std_file"), -1);
	if (LV_SUCCEEDED(lv_get(v, -2))) {
		lv_remove(v, -2); //removes the registry
		lv_pushroottable(v); // push the this
		lv_pushuserpointer(v, file); //file
		if (own) {
			lv_pushinteger(v, 1); //true
		} else {
			lv_pushnull(v); //false
		}
		if (LV_SUCCEEDED(lv_call(v, 3, SQTrue, SQFalse))) {
			lv_remove(v, -2);
			return LV_OK;
		}
	}
	lv_settop(v, top);
	return LV_ERROR;
}

SQRESULT lv_getfile(HSQUIRRELVM v, SQInteger idx, SQFILE *file) {
	SQFile *fileobj = NULL;
	if (LV_SUCCEEDED(lv_getinstanceup(v, idx, (SQUserPointer *)&fileobj, (SQUserPointer)SQ_FILE_TYPE_TAG))) {
		*file = fileobj->GetHandle();
		return LV_OK;
	}
	return lv_throwerror(v, _SC("not a file"));
}
#define IO_BUFFER_SIZE 2048
struct IOBuffer {
	unsigned char buffer[IO_BUFFER_SIZE];
	SQInteger size;
	SQInteger ptr;
	SQFILE file;
};

SQInteger _read_byte(IOBuffer *iobuffer) {
	if (iobuffer->ptr < iobuffer->size) {

		SQInteger ret = iobuffer->buffer[iobuffer->ptr];
		iobuffer->ptr++;
		return ret;
	} else {
		if ( (iobuffer->size = lv_fread(iobuffer->buffer, 1, IO_BUFFER_SIZE, iobuffer->file )) > 0 ) {
			SQInteger ret = iobuffer->buffer[0];
			iobuffer->ptr = 1;
			return ret;
		}
	}

	return 0;
}

SQInteger _read_two_bytes(IOBuffer *iobuffer) {
	if (iobuffer->ptr < iobuffer->size) {
		if (iobuffer->size < 2) return 0;
		SQInteger ret = *((const wchar_t *)&iobuffer->buffer[iobuffer->ptr]);
		iobuffer->ptr += 2;
		return ret;
	} else {
		if ( (iobuffer->size = lv_fread(iobuffer->buffer, 1, IO_BUFFER_SIZE, iobuffer->file )) > 0 ) {
			if (iobuffer->size < 2) return 0;
			SQInteger ret = *((const wchar_t *)&iobuffer->buffer[0]);
			iobuffer->ptr = 2;
			return ret;
		}
	}

	return 0;
}

static SQInteger _io_file_lexfeed_PLAIN(SQUserPointer iobuf) {
	IOBuffer *iobuffer = (IOBuffer *)iobuf;
	return _read_byte(iobuffer);

}

#ifdef SQUNICODE
static SQInteger _io_file_lexfeed_UTF8(SQUserPointer iobuf) {
	IOBuffer *iobuffer = (IOBuffer *)iobuf;
#define READ(iobuf) \
	if((inchar = (unsigned char)_read_byte(iobuf)) == 0) \
		return 0;

	static const SQInteger utf8_lengths[16] = {
		1, 1, 1, 1, 1, 1, 1, 1, /* 0000 to 0111 : 1 byte (plain ASCII) */
		0, 0, 0, 0,             /* 1000 to 1011 : not valid */
		2, 2,                   /* 1100, 1101 : 2 bytes */
		3,                      /* 1110 : 3 bytes */
		4                       /* 1111 :4 bytes */
	};
	static const unsigned char byte_masks[5] = {0, 0, 0x1f, 0x0f, 0x07};
	unsigned char inchar;
	SQInteger c = 0;
	READ(iobuffer);
	c = inchar;
	//
	if (c >= 0x80) {
		SQInteger tmp;
		SQInteger codelen = utf8_lengths[c >> 4];
		if (codelen == 0)
			return 0;
		//"invalid UTF-8 stream";
		tmp = c & byte_masks[codelen];
		for (SQInteger n = 0; n < codelen - 1; n++) {
			tmp <<= 6;
			READ(iobuffer);
			tmp |= inchar & 0x3F;
		}
		c = tmp;
	}
	return c;
}
#endif

static SQInteger _io_file_lexfeed_UCS2_LE(SQUserPointer iobuf) {
	SQInteger ret;
	IOBuffer *iobuffer = (IOBuffer *)iobuf;
	if ( (ret = _read_two_bytes(iobuffer)) > 0 )
		return ret;
	return 0;
}

static SQInteger _io_file_lexfeed_UCS2_BE(SQUserPointer iobuf) {
	SQInteger c;
	IOBuffer *iobuffer = (IOBuffer *)iobuf;
	if ( (c = _read_two_bytes(iobuffer)) > 0 ) {
		c = ((c >> 8) & 0x00FF) | ((c << 8) & 0xFF00);
		return c;
	}
	return 0;
}

SQInteger file_read(SQUserPointer file, SQUserPointer buf, SQInteger size) {
	SQInteger ret;
	if ((ret = lv_fread(buf, 1, size, (SQFILE)file )) != 0)
		return ret;
	return -1;
}

SQInteger file_write(SQUserPointer file, SQUserPointer p, SQInteger size) {
	return lv_fwrite(p, 1, size, (SQFILE)file);
}

SQRESULT lv_loadfile(HSQUIRRELVM v, const SQChar *filename, SQBool printerror) {
	SQFILE file = lv_fopen(filename, _SC("rb"));

	SQInteger ret;
	unsigned short us;
	unsigned char uc;
	SQLEXREADFUNC func = _io_file_lexfeed_PLAIN;
	if (file) {
		ret = lv_fread(&us, 1, 2, file);
		if (ret != 2) {
			//probably an empty file
			us = 0;
		}
		if (us == SQ_BYTECODE_STREAM_TAG) { //BYTECODE
			lv_fseek(file, 0, SQ_SEEK_SET);
			if (LV_SUCCEEDED(lv_readclosure(v, file_read, file))) {
				lv_fclose(file);
				return LV_OK;
			}
		} else { //SCRIPT

			switch (us) {
				//gotta swap the next 2 lines on BIG endian machines
				case 0xFFFE:
					func = _io_file_lexfeed_UCS2_BE;
					break; // UTF-16 little endian
				case 0xFEFF:
					func = _io_file_lexfeed_UCS2_LE;
					break; // UTF-16 big endian
				case 0xBBEF:
					if (lv_fread(&uc, 1, sizeof(uc), file) == 0) {
						lv_fclose(file);
						return lv_throwerror(v, _SC("io error"));
					}
					if (uc != 0xBF) {
						lv_fclose(file);
						return lv_throwerror(v, _SC("Unrecognozed ecoding"));
					}
#ifdef SQUNICODE
					func = _io_file_lexfeed_UTF8;
#else
					func = _io_file_lexfeed_PLAIN;
#endif
					break; // UTF-8
				default:
					lv_fseek(file, 0, SQ_SEEK_SET);
					break; // ascii
			}

			IOBuffer buffer;
			buffer.ptr = 0;
			buffer.size = 0;
			buffer.file = file;
			if (LV_SUCCEEDED(lv_compile(v, func, &buffer, filename, printerror))) {
				lv_fclose(file);
				return LV_OK;
			}
		}
		lv_fclose(file);
		return LV_ERROR;
	}
	return lv_throwerror(v, _SC("cannot open the file"));
}

SQRESULT lv_execfile(HSQUIRRELVM v, const SQChar *filename, SQBool retval, SQBool printerror) {
	if (LV_SUCCEEDED(lv_loadfile(v, filename, printerror))) {
		lv_push(v, -2);
		if (LV_SUCCEEDED(lv_call(v, 1, retval, SQTrue))) {
			lv_remove(v, retval ? -2 : -1); //removes the closure
			return 1;
		}
		lv_pop(v, 1); //removes the closure
	}
	return LV_ERROR;
}

SQRESULT lv_writeclosuretofile(HSQUIRRELVM v, const SQChar *filename) {
	SQFILE file = lv_fopen(filename, _SC("wb+"));
	if (!file)
		return lv_throwerror(v, _SC("cannot open the file"));
	if (LV_SUCCEEDED(lv_writeclosure(v, file_write, file))) {
		lv_fclose(file);
		return LV_OK;
	}
	lv_fclose(file);
	return LV_ERROR; //forward the error
}

SQInteger _g_io_loadfile(HSQUIRRELVM v) {
	const SQChar *filename;
	SQBool printerror = SQFalse;
	lv_getstring(v, 2, &filename);
	if (lv_gettop(v) >= 3) {
		lv_getbool(v, 3, &printerror);
	}
	if (LV_SUCCEEDED(lv_loadfile(v, filename, printerror)))
		return 1;
	return LV_ERROR; //propagates the error
}

SQInteger _g_io_writeclosuretofile(HSQUIRRELVM v) {
	const SQChar *filename;
	lv_getstring(v, 2, &filename);
	if (LV_SUCCEEDED(lv_writeclosuretofile(v, filename)))
		return 1;
	return LV_ERROR; //propagates the error
}

SQInteger _g_io_execfile(HSQUIRRELVM v) {
	const SQChar *filename;
	SQBool printerror = SQFalse;
	lv_getstring(v, 2, &filename);
	if (lv_gettop(v) >= 3) {
		lv_getbool(v, 3, &printerror);
	}
	lv_push(v, 1); //repush the this
	if (LV_SUCCEEDED(lv_execfile(v, filename, SQTrue, printerror)))
		return 1;
	return LV_ERROR; //propagates the error
}

CALLBACK SQInteger callback_loadunit(HSQUIRRELVM v, const SQChar *sSource, SQBool printerror) {
	if (LV_FAILED(lv_execfile(v, sSource, SQTrue, printerror))) {
		return LV_ERROR;
	}
	return LV_OK;
}

#define _DECL_GLOBALIO_FUNC(name,nparams,typecheck) {_SC(#name),_g_io_##name,nparams,typecheck}
static const SQRegFunction iolib_funcs[] = {
	_DECL_GLOBALIO_FUNC(loadfile, -2, _SC(".sb")),
	_DECL_GLOBALIO_FUNC(execfile, -2, _SC(".sb")),
	_DECL_GLOBALIO_FUNC(writeclosuretofile, 3, _SC(".sc")),
	{NULL, (SQFUNCTION)0, 0, NULL}
};

SQRESULT mod_init_io(HSQUIRRELVM v) {
	SQInteger top = lv_gettop(v);
	//create delegate
	declare_stream(v, _SC("file"), (SQUserPointer)SQ_FILE_TYPE_TAG, _SC("std_file"), _file_methods, iolib_funcs);
	lv_pushstring(v, _SC("stdout"), -1);
	lv_createfile(v, stdout, SQFalse);
	lv_newslot(v, -3, SQFalse);
	lv_pushstring(v, _SC("stdin"), -1);
	lv_createfile(v, stdin, SQFalse);
	lv_newslot(v, -3, SQFalse);
	lv_pushstring(v, _SC("stderr"), -1);
	lv_createfile(v, stderr, SQFalse);
	lv_newslot(v, -3, SQFalse);
	lv_settop(v, top);

	lv_setunitloader(v, callback_loadunit);

	return LV_OK;
}
