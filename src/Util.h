void* MovePointer(char** ptr, int offset);
void* MovePointerStep(char** ptr, int step, int offset);

typedef struct VBuffer_ {
	char* ptr;
	size_t maxLen;
	size_t len;
} VBuffer;

VBuffer VBCreate(size_t size);
int VBAppend(VBuffer* buf, char* source, size_t size);
int VBReplace(VBuffer* buf, char* source, size_t size);
void VBDestroy(VBuffer* buf);
