#ifndef _SQSTDIO_H_
#define _SQSTDIO_H_

#ifdef __cplusplus

#define SQSTD_STREAM_TYPE_TAG 0x80000000

struct SQStream {
	virtual SQInteger Read(void *buffer, SQInteger size) = 0;
	virtual SQInteger Write(void *buffer, SQInteger size) = 0;
	virtual SQInteger Flush() = 0;
	virtual SQInteger Tell() = 0;
	virtual SQInteger Len() = 0;
	virtual SQInteger Seek(SQInteger offset, SQInteger origin) = 0;
	virtual bool IsValid() = 0;
	virtual bool EOS() = 0;
};

extern "C" {
#endif

#define SQ_SEEK_CUR 0
#define SQ_SEEK_END 1
#define SQ_SEEK_SET 2

typedef void *SQFILE;

LAVRIL_API SQFILE sqstd_fopen(const SQChar *, const SQChar *);
LAVRIL_API SQInteger sqstd_fread(SQUserPointer, SQInteger, SQInteger, SQFILE);
LAVRIL_API SQInteger sqstd_fwrite(const SQUserPointer, SQInteger, SQInteger, SQFILE);
LAVRIL_API SQInteger sqstd_fseek(SQFILE , SQInteger , SQInteger);
LAVRIL_API SQInteger sqstd_ftell(SQFILE);
LAVRIL_API SQInteger sqstd_fflush(SQFILE);
LAVRIL_API SQInteger sqstd_fclose(SQFILE);
LAVRIL_API SQInteger sqstd_feof(SQFILE);

LAVRIL_API SQRESULT sqstd_createfile(HSQUIRRELVM v, SQFILE file, SQBool own);
LAVRIL_API SQRESULT sqstd_getfile(HSQUIRRELVM v, SQInteger idx, SQFILE *file);

//compiler helpers
LAVRIL_API SQRESULT sqstd_loadfile(HSQUIRRELVM v, const SQChar *filename, SQBool printerror);
LAVRIL_API SQRESULT sqstd_execfile(HSQUIRRELVM v, const SQChar *filename, SQBool retval, SQBool printerror);
LAVRIL_API SQRESULT sqstd_writeclosuretofile(HSQUIRRELVM v, const SQChar *filename);

LAVRIL_API SQRESULT sqstd_register_iolib(HSQUIRRELVM v);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*_SQSTDIO_H_*/

