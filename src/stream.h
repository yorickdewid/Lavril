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

SQInteger _stream_readblob(VMHANDLE v);
SQInteger _stream_readline(VMHANDLE v);
SQInteger _stream_readn(VMHANDLE v);
SQInteger _stream_writeblob(VMHANDLE v);
SQInteger _stream_writen(VMHANDLE v);
SQInteger _stream_seek(VMHANDLE v);
SQInteger _stream_tell(VMHANDLE v);
SQInteger _stream_len(VMHANDLE v);
SQInteger _stream_eos(VMHANDLE v);
SQInteger _stream_flush(VMHANDLE v);

#define _DECL_STREAM_FUNC(name,nparams,typecheck) {_LC(#name),_stream_##name,nparams,typecheck}
SQRESULT declare_stream(VMHANDLE v, const SQChar *name, SQUserPointer typetag, const SQChar *reg_name, const SQRegFunction *methods, const SQRegFunction *globals);
#endif // _STREAM_H_
