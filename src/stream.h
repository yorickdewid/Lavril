#ifndef _STREAM_H_
#define _STREAM_H_

struct SQStream {
	virtual LVInteger Read(void *buffer, LVInteger size) = 0;
	virtual LVInteger Write(void *buffer, LVInteger size) = 0;
	virtual LVInteger Flush() = 0;
	virtual LVInteger Tell() = 0;
	virtual LVInteger Len() = 0;
	virtual LVInteger Seek(LVInteger offset, LVInteger origin) = 0;
	virtual bool IsValid() = 0;
	virtual bool EOS() = 0;
};

LVInteger _stream_readblob(VMHANDLE v);
LVInteger _stream_readline(VMHANDLE v);
LVInteger _stream_readn(VMHANDLE v);
LVInteger _stream_writeblob(VMHANDLE v);
LVInteger _stream_writen(VMHANDLE v);
LVInteger _stream_seek(VMHANDLE v);
LVInteger _stream_tell(VMHANDLE v);
LVInteger _stream_len(VMHANDLE v);
LVInteger _stream_eos(VMHANDLE v);
LVInteger _stream_flush(VMHANDLE v);

#define _DECL_STREAM_FUNC(name,nparams,typecheck) {_LC(#name),_stream_##name,nparams,typecheck}
LVRESULT declare_stream(VMHANDLE v, const LVChar *name, LVUserPointer typetag, const LVChar *reg_name, const SQRegFunction *methods, const SQRegFunction *globals);
#endif // _STREAM_H_
