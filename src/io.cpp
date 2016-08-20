#include "pcheader.h"
#include "stream.h"

#define FILE_TYPE_TAG (STREAM_TYPE_TAG | 0x00000001)

LVFILE lv_fopen(const LVChar *filename , const LVChar *mode) {
#ifndef LVUNICODE
	return (LVFILE)fopen(filename, mode);
#else
	return (LVFILE)_wfopen(filename, mode);
#endif
}

LVInteger lv_fread(void *buffer, LVInteger size, LVInteger count, LVFILE file) {
	LVInteger ret = (LVInteger)fread(buffer, size, count, (FILE *)file);
	return ret;
}

LVInteger lv_fwrite(const LVUserPointer buffer, LVInteger size, LVInteger count, LVFILE file) {
	return (LVInteger)fwrite(buffer, size, count, (FILE *)file);
}

LVInteger lv_fseek(LVFILE file, LVInteger offset, LVInteger origin) {
	LVInteger realorigin;
	switch (origin) {
		case LV_SEEK_CUR:
			realorigin = SEEK_CUR;
			break;
		case LV_SEEK_END:
			realorigin = SEEK_END;
			break;
		case LV_SEEK_SET:
			realorigin = SEEK_SET;
			break;
		default:
			return -1; //failed
	}
	return fseek((FILE *)file, (long)offset, (int)realorigin);
}

LVInteger lv_ftell(LVFILE file) {
	return ftell((FILE *)file);
}

LVInteger lv_fflush(LVFILE file) {
	return fflush((FILE *)file);
}

LVInteger lv_fclose(LVFILE file) {
	return fclose((FILE *)file);
}

LVInteger lv_feof(LVFILE file) {
	return feof((FILE *)file);
}

//File
struct LVFile : public LVStream {
	LVFile() {
		_handle = NULL;
		_owns = false;
	}
	LVFile(LVFILE file, bool owns) {
		_handle = file;
		_owns = owns;
	}
	virtual ~LVFile() {
		Close();
	}
	bool Open(const LVChar *filename , const LVChar *mode) {
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
	LVInteger Read(void *buffer, LVInteger size) {
		return lv_fread(buffer, 1, size, _handle);
	}
	LVInteger Write(void *buffer, LVInteger size) {
		return lv_fwrite(buffer, 1, size, _handle);
	}
	LVInteger Flush() {
		return lv_fflush(_handle);
	}
	LVInteger Tell() {
		return lv_ftell(_handle);
	}
	LVInteger Len() {
		LVInteger prevpos = Tell();
		Seek(0, LV_SEEK_END);
		LVInteger size = Tell();
		Seek(prevpos, LV_SEEK_SET);
		return size;
	}
	LVInteger Seek(LVInteger offset, LVInteger origin)  {
		return lv_fseek(_handle, offset, origin);
	}
	bool IsValid() {
		return _handle ? true : false;
	}
	bool EOS() {
		return Tell() == Len() ? true : false;
	}
	LVFILE GetHandle() {
		return _handle;
	}
  private:
	LVFILE _handle;
	bool _owns;
};

static LVInteger _file__typeof(VMHANDLE v) {
	lv_pushstring(v, _LC("file"), -1);
	return 1;
}

static LVInteger _file_releasehook(LVUserPointer p, LVInteger LV_UNUSED_ARG(size)) {
	LVFile *self = (LVFile *)p;
	self->~LVFile();
	lv_free(self, sizeof(LVFile));
	return 1;
}

static LVInteger _file_constructor(VMHANDLE v) {
	const LVChar *filename, *mode;
	bool owns = true;
	LVFile *f;
	LVFILE newf;
	if (lv_gettype(v, 2) == OT_STRING && lv_gettype(v, 3) == OT_STRING) {
		lv_getstring(v, 2, &filename);
		lv_getstring(v, 3, &mode);
		newf = lv_fopen(filename, mode);
		if (!newf)
			return lv_throwerror(v, _LC("cannot open file"));
	} else if (lv_gettype(v, 2) == OT_USERPOINTER) {
		owns = !(lv_gettype(v, 3) == OT_NULL);
		lv_getuserpointer(v, 2, &newf);
	} else {
		return lv_throwerror(v, _LC("wrong parameter"));
	}

	f = new(lv_malloc(sizeof(LVFile))) LVFile(newf, owns);
	if (LV_FAILED(lv_setinstanceup(v, 1, f))) {
		f->~LVFile();
		lv_free(f, sizeof(LVFile));
		return lv_throwerror(v, _LC("cannot create blob with negative size"));
	}
	lv_setreleasehook(v, 1, _file_releasehook);
	return 0;
}

static LVInteger _file_close(VMHANDLE v) {
	LVFile *self = NULL;
	if (LV_SUCCEEDED(lv_getinstanceup(v, 1, (LVUserPointer *)&self, (LVUserPointer)FILE_TYPE_TAG)) && self != NULL) {
		self->Close();
	}
	return 0;
}

#define _DECL_FILE_FUNC(name,nparams,typecheck) {_LC(#name),_file_##name,nparams,typecheck}
static const LVRegFunction _file_methods[] = {
	_DECL_FILE_FUNC(constructor, 3, _LC("x")),
	_DECL_FILE_FUNC(_typeof, 1, _LC("x")),
	_DECL_FILE_FUNC(close, 1, _LC("x")),
	{NULL, (LVFUNCTION)0, 0, NULL}
};

LVRESULT lv_createfile(VMHANDLE v, LVFILE file, LVBool own) {
	LVInteger top = lv_gettop(v);
	lv_pushregistrytable(v);
	lv_pushstring(v, _LC("std_file"), -1);
	if (LV_SUCCEEDED(lv_get(v, -2))) {
		lv_remove(v, -2); //removes the registry
		lv_pushroottable(v); // push the this
		lv_pushuserpointer(v, file); //file
		if (own) {
			lv_pushinteger(v, 1); //true
		} else {
			lv_pushnull(v); //false
		}
		if (LV_SUCCEEDED(lv_call(v, 3, LVTrue, LVFalse))) {
			lv_remove(v, -2);
			return LV_OK;
		}
	}
	lv_settop(v, top);
	return LV_ERROR;
}

LVRESULT lv_getfile(VMHANDLE v, LVInteger idx, LVFILE *file) {
	LVFile *fileobj = NULL;
	if (LV_SUCCEEDED(lv_getinstanceup(v, idx, (LVUserPointer *)&fileobj, (LVUserPointer)FILE_TYPE_TAG))) {
		*file = fileobj->GetHandle();
		return LV_OK;
	}
	return lv_throwerror(v, _LC("not a file"));
}
#define IO_BUFFER_SIZE 2048
struct IOBuffer {
	unsigned char buffer[IO_BUFFER_SIZE];
	LVInteger size;
	LVInteger ptr;
	LVFILE file;
};

LVInteger _read_byte(IOBuffer *iobuffer) {
	if (iobuffer->ptr < iobuffer->size) {

		LVInteger ret = iobuffer->buffer[iobuffer->ptr];
		iobuffer->ptr++;
		return ret;
	} else {
		if ( (iobuffer->size = lv_fread(iobuffer->buffer, 1, IO_BUFFER_SIZE, iobuffer->file )) > 0 ) {
			LVInteger ret = iobuffer->buffer[0];
			iobuffer->ptr = 1;
			return ret;
		}
	}

	return 0;
}

LVInteger _read_two_bytes(IOBuffer *iobuffer) {
	if (iobuffer->ptr < iobuffer->size) {
		if (iobuffer->size < 2) return 0;
		LVInteger ret = *((const wchar_t *)&iobuffer->buffer[iobuffer->ptr]);
		iobuffer->ptr += 2;
		return ret;
	} else {
		if ( (iobuffer->size = lv_fread(iobuffer->buffer, 1, IO_BUFFER_SIZE, iobuffer->file )) > 0 ) {
			if (iobuffer->size < 2) return 0;
			LVInteger ret = *((const wchar_t *)&iobuffer->buffer[0]);
			iobuffer->ptr = 2;
			return ret;
		}
	}

	return 0;
}

static LVInteger _io_file_lexfeed_PLAIN(LVUserPointer iobuf) {
	IOBuffer *iobuffer = (IOBuffer *)iobuf;
	return _read_byte(iobuffer);

}

#ifdef LVUNICODE
static LVInteger _io_file_lexfeed_UTF8(LVUserPointer iobuf) {
	IOBuffer *iobuffer = (IOBuffer *)iobuf;
#define READ(iobuf) \
	if((inchar = (unsigned char)_read_byte(iobuf)) == 0) \
		return 0;

	static const LVInteger utf8_lengths[16] = {
		1, 1, 1, 1, 1, 1, 1, 1, /* 0000 to 0111 : 1 byte (plain ASCII) */
		0, 0, 0, 0,             /* 1000 to 1011 : not valid */
		2, 2,                   /* 1100, 1101 : 2 bytes */
		3,                      /* 1110 : 3 bytes */
		4                       /* 1111 :4 bytes */
	};
	static const unsigned char byte_masks[5] = {0, 0, 0x1f, 0x0f, 0x07};
	unsigned char inchar;
	LVInteger c = 0;
	READ(iobuffer);
	c = inchar;
	//
	if (c >= 0x80) {
		LVInteger tmp;
		LVInteger codelen = utf8_lengths[c >> 4];
		if (codelen == 0)
			return 0;
		//"invalid UTF-8 stream";
		tmp = c & byte_masks[codelen];
		for (LVInteger n = 0; n < codelen - 1; n++) {
			tmp <<= 6;
			READ(iobuffer);
			tmp |= inchar & 0x3F;
		}
		c = tmp;
	}
	return c;
}
#endif

static LVInteger _io_file_lexfeed_UCS2_LE(LVUserPointer iobuf) {
	LVInteger ret;
	IOBuffer *iobuffer = (IOBuffer *)iobuf;
	if ( (ret = _read_two_bytes(iobuffer)) > 0 )
		return ret;
	return 0;
}

static LVInteger _io_file_lexfeed_UCS2_BE(LVUserPointer iobuf) {
	LVInteger c;
	IOBuffer *iobuffer = (IOBuffer *)iobuf;
	if ( (c = _read_two_bytes(iobuffer)) > 0 ) {
		c = ((c >> 8) & 0x00FF) | ((c << 8) & 0xFF00);
		return c;
	}
	return 0;
}

LVInteger file_read(LVUserPointer file, LVUserPointer buf, LVInteger size) {
	LVInteger ret;
	if ((ret = lv_fread(buf, 1, size, (LVFILE)file )) != 0)
		return ret;
	return -1;
}

LVInteger file_write(LVUserPointer file, LVUserPointer p, LVInteger size) {
	return lv_fwrite(p, 1, size, (LVFILE)file);
}

LVRESULT lv_loadfile(VMHANDLE v, const LVChar *filename, LVBool printerror) {
	LVFILE file = lv_fopen(filename, _LC("rb"));

	LVInteger ret;
	unsigned short us;
	unsigned char uc;
	LVLEXREADFUNC func = _io_file_lexfeed_PLAIN;
	if (file) {
		ret = lv_fread(&us, 1, 2, file);
		if (ret != 2) {
			//probably an empty file
			us = 0;
		}
		if (us == BYTECODE_STREAM_TAG) { //BYTECODE
			lv_fseek(file, 0, LV_SEEK_SET);
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
						return lv_throwerror(v, _LC("io error"));
					}
					if (uc != 0xBF) {
						lv_fclose(file);
						return lv_throwerror(v, _LC("Unrecognozed ecoding"));
					}
#ifdef LVUNICODE
					func = _io_file_lexfeed_UTF8;
#else
					func = _io_file_lexfeed_PLAIN;
#endif
					break; // UTF-8
				default:
					lv_fseek(file, 0, LV_SEEK_SET);
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
	return lv_throwerror(v, _LC("cannot open the file"));
}

LVRESULT lv_execfile(VMHANDLE v, const LVChar *filename, LVBool retval, LVBool printerror) {
	if (LV_SUCCEEDED(lv_loadfile(v, filename, printerror))) {
		lv_push(v, -2);
		if (LV_SUCCEEDED(lv_call(v, 1, retval, LVTrue))) {
			lv_remove(v, retval ? -2 : -1); //removes the closure
			return 1;
		}
		lv_pop(v, 1); //removes the closure
	}
	return LV_ERROR;
}

LVRESULT lv_writeclosuretofile(VMHANDLE v, const LVChar *filename) {
	LVFILE file = lv_fopen(filename, _LC("wb+"));
	if (!file)
		return lv_throwerror(v, _LC("cannot open the file"));
	if (LV_SUCCEEDED(lv_writeclosure(v, file_write, file))) {
		lv_fclose(file);
		return LV_OK;
	}
	lv_fclose(file);
	return LV_ERROR; //forward the error
}

LVInteger _g_io_loadfile(VMHANDLE v) {
	const LVChar *filename;
	LVBool printerror = LVFalse;
	lv_getstring(v, 2, &filename);
	if (lv_gettop(v) >= 3) {
		lv_getbool(v, 3, &printerror);
	}
	if (LV_SUCCEEDED(lv_loadfile(v, filename, printerror)))
		return 1;
	return LV_ERROR; //propagates the error
}

LVInteger _g_io_writeclosuretofile(VMHANDLE v) {
	const LVChar *filename;
	lv_getstring(v, 2, &filename);
	if (LV_SUCCEEDED(lv_writeclosuretofile(v, filename)))
		return 1;
	return LV_ERROR; //propagates the error
}

LVInteger _g_io_execfile(VMHANDLE v) {
	const LVChar *filename;
	LVBool printerror = LVFalse;
	lv_getstring(v, 2, &filename);
	if (lv_gettop(v) >= 3) {
		lv_getbool(v, 3, &printerror);
	}
	lv_push(v, 1); //repush the this
	if (LV_SUCCEEDED(lv_execfile(v, filename, LVTrue, printerror)))
		return 1;
	return LV_ERROR; //propagates the error
}

CALLBACK LVInteger callback_loadunit(VMHANDLE v, const LVChar *sSource, LVBool printerror) {
	if (LV_FAILED(lv_execfile(v, sSource, LVTrue, printerror))) {
		return LV_ERROR;
	}
	return LV_OK;
}

#define _DECL_GLOBALIO_FUNC(name,nparams,typecheck) {_LC(#name),_g_io_##name,nparams,typecheck}
#define _DECL_GLOBALIO_ALIAS(alias,name,nparams,typecheck) {_LC(#alias),_g_io_##name,nparams,typecheck}
static const LVRegFunction iolib_funcs[] = {
	_DECL_GLOBALIO_FUNC(loadfile, -2, _LC(".sb")),
	_DECL_GLOBALIO_FUNC(execfile, -2, _LC(".sb")),
	_DECL_GLOBALIO_ALIAS(include, execfile, -2, _LC(".sb")),
	_DECL_GLOBALIO_ALIAS(require, execfile, -2, _LC(".sb")),
	_DECL_GLOBALIO_ALIAS(import, execfile, -2, _LC(".sb")),
	_DECL_GLOBALIO_FUNC(writeclosuretofile, 3, _LC(".sc")),
	{NULL, (LVFUNCTION)0, 0, NULL}
};

LVRESULT mod_init_io(VMHANDLE v) {
	LVInteger top = lv_gettop(v);

	/* Create delegate */
	declare_stream(v, _LC("file"), (LVUserPointer)FILE_TYPE_TAG, _LC("std_file"), _file_methods, iolib_funcs);

	lv_pushstring(v, _LC("stdout"), -1);
	lv_createfile(v, stdout, LVFalse);
	lv_newslot(v, -3, LVFalse);
	lv_pushstring(v, _LC("stdin"), -1);
	lv_createfile(v, stdin, LVFalse);
	lv_newslot(v, -3, LVFalse);
	lv_pushstring(v, _LC("stderr"), -1);
	lv_createfile(v, stderr, LVFalse);
	lv_newslot(v, -3, LVFalse);
	lv_settop(v, top);

	return LV_OK;
}
