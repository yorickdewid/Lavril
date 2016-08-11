#include "pcheader.h"

#ifndef SQ_EXCLUDE_DEFAULT_MEMFUNCTIONS

void *lv_vm_malloc(SQUnsignedInteger size) {
	return malloc(size);
}

void *lv_vm_realloc(void *p, SQUnsignedInteger LV_UNUSED_ARG(oldsize), SQUnsignedInteger size) {
	return realloc(p, size);
}

void lv_vm_free(void *p, SQUnsignedInteger LV_UNUSED_ARG(size)) {
	free(p);
}

#endif
