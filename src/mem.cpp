#include "pcheader.h"

#ifndef SQ_EXCLUDE_DEFAULT_MEMFUNCTIONS

void *lv_vm_malloc(LVUnsignedInteger size) {
	return malloc(size);
}

void *lv_vm_realloc(void *p, LVUnsignedInteger LV_UNUSED_ARG(oldsize), LVUnsignedInteger size) {
	return realloc(p, size);
}

void lv_vm_free(void *p, LVUnsignedInteger LV_UNUSED_ARG(size)) {
	free(p);
}

#endif
