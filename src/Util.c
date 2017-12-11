#include <stdlib.h>
#include <string.h>
#include "Util.h"

void* MovePointer(char** ptr, int offset) {
	if (ptr == 0) return 0;
	char* ret = *ptr;
	*ptr += offset;
	return ret;
}

void* MovePointerStep(char** ptr, int step, int count) {
	return MovePointer(ptr, step*count);
}

VBuffer VBCreate(size_t size) {
	VBuffer vb;
	if (size == 0) {
		memset(&vb, 0, sizeof(vb));
		return vb;
	}

	vb.ptr = (char*)malloc(size);
	vb.maxLen = size;
	vb.len = 0;

	return vb;
}

int VBAppend(VBuffer* buf, void* source, size_t size) {
	if (buf->ptr == NULL) return 0;
	if (size == 0) return 1;

	if ((buf->maxLen - buf->len) < size) {
		size_t newSize = (size>buf->maxLen*2)?(int)size*1.5:buf->maxLen*2;
		buf->maxLen = newSize;
		buf->ptr = (char*)realloc(buf->ptr, newSize);
	}
	if (source) {
		memcpy(buf->ptr + buf->len, source, size);
		buf->len += size;
	}

	return 1;
}

int VBReplace(VBuffer* buf, void* source, size_t size) {
	if (buf->ptr == NULL) return 0;
	if (source == NULL) return 0;
	if (size == 0) return 1;
	
	if (buf->maxLen < size) {
		size_t newSize = (size>buf->maxLen*2)?(int)size*1.5:buf->maxLen*2;
		buf->maxLen = newSize;
		free(buf->ptr);
		buf->ptr = (char*)malloc(newSize);
	}
	memcpy(buf->ptr, source, size);
	buf->len = size;
	return 1;
}

void VBDestroy(VBuffer* buf) {
	if (buf->ptr) {
		free(buf->ptr);
	}
	memset(buf, 0, sizeof(*buf));
}

int VBClear(VBuffer* buf) {
	if (buf->ptr) {
		buf->len = 0;
		return 1;
	}
	return 0;
}
