#ifndef _STREAM_H_
#define _STREAM_H_

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

SQInteger _stream_readblob(HSQUIRRELVM v);
SQInteger _stream_readline(HSQUIRRELVM v);
SQInteger _stream_readn(HSQUIRRELVM v);
SQInteger _stream_writeblob(HSQUIRRELVM v);
SQInteger _stream_writen(HSQUIRRELVM v);
SQInteger _stream_seek(HSQUIRRELVM v);
SQInteger _stream_tell(HSQUIRRELVM v);
SQInteger _stream_len(HSQUIRRELVM v);
SQInteger _stream_eos(HSQUIRRELVM v);
SQInteger _stream_flush(HSQUIRRELVM v);

#define _DECL_STREAM_FUNC(name,nparams,typecheck) {_LC(#name),_stream_##name,nparams,typecheck}
SQRESULT declare_stream(HSQUIRRELVM v, const SQChar *name, SQUserPointer typetag, const SQChar *reg_name, const SQRegFunction *methods, const SQRegFunction *globals);
#endif // _STREAM_H_
