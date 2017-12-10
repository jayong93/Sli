#include <pthread.h>
#include "ClientCommu.h"

pthread_mutex_t rDataLock;

void* RecvMsg() {
#ifdef USE_FIFO
#else
#endif
}

void* SendMsg() {
#ifdef USE_FIFO
#else
#endif
}
