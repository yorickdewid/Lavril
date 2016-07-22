#ifndef _SQSTDBLOB_H_
#define _SQSTDBLOB_H_

#ifdef __cplusplus
extern "C" {
#endif

LAVRIL_API SQUserPointer sqstd_createblob(HSQUIRRELVM v, SQInteger size);
LAVRIL_API SQRESULT sqstd_getblob(HSQUIRRELVM v, SQInteger idx, SQUserPointer *ptr);
LAVRIL_API SQInteger sqstd_getblobsize(HSQUIRRELVM v, SQInteger idx);

LAVRIL_API SQRESULT sqstd_register_bloblib(HSQUIRRELVM v);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*_SQSTDBLOB_H_*/

