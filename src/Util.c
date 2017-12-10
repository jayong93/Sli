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
