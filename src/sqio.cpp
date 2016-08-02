#include "sqpcheader.h"
#include "sqstream.h"

#define SQ_FILE_TYPE_TAG (SQ_STREAM_TYPE_TAG | 0x00000001)

SQFILE sq_fopen(const SQChar *filename , const SQChar *mode) {
#ifndef SQUNICODE
	return (SQFILE)fopen(filename, mode);
#else
	return (SQFILE)_wfopen(filename, mode);
#endif
}

SQInteger sq_fread(void *buffer, SQInteger size, SQInteger count, SQFILE file) {
	SQInteger ret = (SQInteger)fread(buffer, size, count, (FILE *)file);
	return ret;
}

SQInteger sq_fwrite(const SQUserPointer buffer, SQInteger size, SQInteger count, SQFILE file) {
	return (SQInteger)fwrite(buffer, size, count, (FILE *)file);
}

SQInteger sq_fseek(SQFILE file, SQInteger offset, SQInteger origin) {
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

SQInteger sq_ftell(SQFILE file) {
	return ftell((FILE *)file);
}

SQInteger sq_fflush(SQFILE file) {
	return fflush((FILE *)file);
}

SQInteger sq_fclose(SQFILE file) {
	return fclose((FILE *)file);
}

SQInteger sq_feof(SQFILE file) {
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
		if ( (_handle = sq_fopen(filename, mode)) ) {
			_owns = true;
			return true;
		}
		return false;
	}
	void Close() {
		if (_handle && _owns) {
			sq_fclose(_handle);
			_handle = NULL;
			_owns = false;
		}
	}
	SQInteger Read(void *buffer, SQInteger size) {
		return sq_fread(buffer, 1, size, _handle);
	}
	SQInteger Write(void *buffer, SQInteger size) {
		return sq_fwrite(buffer, 1, size, _handle);
	}
	SQInteger Flush() {
		return sq_fflush(_handle);
	}
	SQInteger Tell() {
		return sq_ftell(_handle);
	}
	SQInteger Len() {
		SQInteger prevpos = Tell();
		Seek(0, SQ_SEEK_END);
		SQInteger size = Tell();
		Seek(prevpos, SQ_SEEK_SET);
		return size;
	}
	SQInteger Seek(SQInteger offset, SQInteger origin)  {
		return sq_fseek(_handle, offset, origin);
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
	sq_pushstring(v, _SC("file"), -1);
	return 1;
}

static SQInteger _file_releasehook(SQUserPointer p, SQInteger SQ_UNUSED_ARG(size)) {
	SQFile *self = (SQFile *)p;
	self->~SQFile();
	sq_free(self, sizeof(SQFile));
	return 1;
}

static SQInteger _file_constructor(HSQUIRRELVM v) {
	const SQChar *filename, *mode;
	bool owns = true;
	SQFile *f;
	SQFILE newf;
	if (sq_gettype(v, 2) == OT_STRING && sq_gettype(v, 3) == OT_STRING) {
		sq_getstring(v, 2, &filename);
		sq_getstring(v, 3, &mode);
		newf = sq_fopen(filename, mode);
		if (!newf) return sq_throwerror(v, _SC("cannot open file"));
	} else if (sq_gettype(v, 2) == OT_USERPOINTER) {
		owns = !(sq_gettype(v, 3) == OT_NULL);
		sq_getuserpointer(v, 2, &newf);
	} else {
		return sq_throwerror(v, _SC("wrong parameter"));
	}

	f = new (sq_malloc(sizeof(SQFile)))SQFile(newf, owns);
	if (LV_FAILED(sq_setinstanceup(v, 1, f))) {
		f->~SQFile();
		sq_free(f, sizeof(SQFile));
		return sq_throwerror(v, _SC("cannot create blob with negative size"));
	}
	sq_setreleasehook(v, 1, _file_releasehook);
	return 0;
}

static SQInteger _file_close(HSQUIRRELVM v) {
	SQFile *self = NULL;
	if (LV_SUCCEEDED(sq_getinstanceup(v, 1, (SQUserPointer *)&self, (SQUserPointer)SQ_FILE_TYPE_TAG))
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

SQRESULT sq_createfile(HSQUIRRELVM v, SQFILE file, SQBool own) {
	SQInteger top = sq_gettop(v);
	sq_pushregistrytable(v);
	sq_pushstring(v, _SC("std_file"), -1);
	if (LV_SUCCEEDED(sq_get(v, -2))) {
		sq_remove(v, -2); //removes the registry
		sq_pushroottable(v); // push the this
		sq_pushuserpointer(v, file); //file
		if (own) {
			sq_pushinteger(v, 1); //true
		} else {
			sq_pushnull(v); //false
		}
		if (LV_SUCCEEDED( sq_call(v, 3, SQTrue, SQFalse) )) {
			sq_remove(v, -2);
			return LV_OK;
		}
	}
	sq_settop(v, top);
	return LV_ERROR;
}

SQRESULT sq_getfile(HSQUIRRELVM v, SQInteger idx, SQFILE *file) {
	SQFile *fileobj = NULL;
	if (LV_SUCCEEDED(sq_getinstanceup(v, idx, (SQUserPointer *)&fileobj, (SQUserPointer)SQ_FILE_TYPE_TAG))) {
		*file = fileobj->GetHandle();
		return LV_OK;
	}
	return sq_throwerror(v, _SC("not a file"));
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
		if ( (iobuffer->size = sq_fread(iobuffer->buffer, 1, IO_BUFFER_SIZE, iobuffer->file )) > 0 ) {
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
		if ( (iobuffer->size = sq_fread(iobuffer->buffer, 1, IO_BUFFER_SIZE, iobuffer->file )) > 0 ) {
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
	if ((ret = sq_fread(buf, 1, size, (SQFILE)file )) != 0)
		return ret;
	return -1;
}

SQInteger file_write(SQUserPointer file, SQUserPointer p, SQInteger size) {
	return sq_fwrite(p, 1, size, (SQFILE)file);
}

SQRESULT sq_loadfile(HSQUIRRELVM v, const SQChar *filename, SQBool printerror) {
	SQFILE file = sq_fopen(filename, _SC("rb"));

	SQInteger ret;
	unsigned short us;
	unsigned char uc;
	SQLEXREADFUNC func = _io_file_lexfeed_PLAIN;
	if (file) {
		ret = sq_fread(&us, 1, 2, file);
		if (ret != 2) {
			//probably an empty file
			us = 0;
		}
		if (us == SQ_BYTECODE_STREAM_TAG) { //BYTECODE
			sq_fseek(file, 0, SQ_SEEK_SET);
			if (LV_SUCCEEDED(sq_readclosure(v, file_read, file))) {
				sq_fclose(file);
				return LV_OK;
			}
		} else { //SCRIPT

			switch (us) {
				//gotta swap the next 2 lines on BIG endian machines
				case 0xFFFE:
					func = _io_file_lexfeed_UCS2_BE;
					break;//UTF-16 little endian;
				case 0xFEFF:
					func = _io_file_lexfeed_UCS2_LE;
					break;//UTF-16 big endian;
				case 0xBBEF:
					if (sq_fread(&uc, 1, sizeof(uc), file) == 0) {
						sq_fclose(file);
						return sq_throwerror(v, _SC("io error"));
					}
					if (uc != 0xBF) {
						sq_fclose(file);
						return sq_throwerror(v, _SC("Unrecognozed ecoding"));
					}
#ifdef SQUNICODE
					func = _io_file_lexfeed_UTF8;
#else
					func = _io_file_lexfeed_PLAIN;
#endif
					break;//UTF-8 ;
				default:
					sq_fseek(file, 0, SQ_SEEK_SET);
					break; // ascii
			}

			IOBuffer buffer;
			buffer.ptr = 0;
			buffer.size = 0;
			buffer.file = file;
			if (LV_SUCCEEDED(sq_compile(v, func, &buffer, filename, printerror))) {
				sq_fclose(file);
				return LV_OK;
			}
		}
		sq_fclose(file);
		return LV_ERROR;
	}
	return sq_throwerror(v, _SC("cannot open the file"));
}

SQRESULT sq_execfile(HSQUIRRELVM v, const SQChar *filename, SQBool retval, SQBool printerror) {
	if (LV_SUCCEEDED(sq_loadfile(v, filename, printerror))) {
		sq_push(v, -2);
		if (LV_SUCCEEDED(sq_call(v, 1, retval, SQTrue))) {
			sq_remove(v, retval ? -2 : -1); //removes the closure
			return 1;
		}
		sq_pop(v, 1); //removes the closure
	}
	return LV_ERROR;
}

SQRESULT sq_writeclosuretofile(HSQUIRRELVM v, const SQChar *filename) {
	SQFILE file = sq_fopen(filename, _SC("wb+"));
	if (!file) return sq_throwerror(v, _SC("cannot open the file"));
	if (LV_SUCCEEDED(sq_writeclosure(v, file_write, file))) {
		sq_fclose(file);
		return LV_OK;
	}
	sq_fclose(file);
	return LV_ERROR; //forward the error
}

SQInteger _g_io_loadfile(HSQUIRRELVM v) {
	const SQChar *filename;
	SQBool printerror = SQFalse;
	sq_getstring(v, 2, &filename);
	if (sq_gettop(v) >= 3) {
		sq_getbool(v, 3, &printerror);
	}
	if (LV_SUCCEEDED(sq_loadfile(v, filename, printerror)))
		return 1;
	return LV_ERROR; //propagates the error
}

SQInteger _g_io_writeclosuretofile(HSQUIRRELVM v) {
	const SQChar *filename;
	sq_getstring(v, 2, &filename);
	if (LV_SUCCEEDED(sq_writeclosuretofile(v, filename)))
		return 1;
	return LV_ERROR; //propagates the error
}

SQInteger _g_io_execfile(HSQUIRRELVM v) {
	const SQChar *filename;
	SQBool printerror = SQFalse;
	sq_getstring(v, 2, &filename);
	if (sq_gettop(v) >= 3) {
		sq_getbool(v, 3, &printerror);
	}
	sq_push(v, 1); //repush the this
	if (LV_SUCCEEDED(sq_execfile(v, filename, SQTrue, printerror)))
		return 1;
	return LV_ERROR; //propagates the error
}

CALLBACK SQInteger callback_loadunit(HSQUIRRELVM v, const SQChar *sSource, SQBool printerror) {
	if (LV_FAILED(sq_execfile(v, sSource, SQTrue, printerror))) {
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
	SQInteger top = sq_gettop(v);
	//create delegate
	declare_stream(v, _SC("file"), (SQUserPointer)SQ_FILE_TYPE_TAG, _SC("std_file"), _file_methods, iolib_funcs);
	sq_pushstring(v, _SC("stdout"), -1);
	sq_createfile(v, stdout, SQFalse);
	sq_newslot(v, -3, SQFalse);
	sq_pushstring(v, _SC("stdin"), -1);
	sq_createfile(v, stdin, SQFalse);
	sq_newslot(v, -3, SQFalse);
	sq_pushstring(v, _SC("stderr"), -1);
	sq_createfile(v, stderr, SQFalse);
	sq_newslot(v, -3, SQFalse);
	sq_settop(v, top);

	sq_setunitloader(v, callback_loadunit);

	return LV_OK;
}