#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <ncursesw/curses.h>
#include "Render.h"
#include "ClientCommu.h"

pthread_mutex_t rDataLock;
pthread_cond_t dataCopyCond;

int main(int argc, char* argv[]){
	if (argc < 2) {
		fprintf(stderr, "not enough arguments\n");
		return 1;
	}
	if (strlen(argv[1]) > 90) {
		fprintf(stderr, "the ID is too long\n");
		return 2;
	}

	if (ConnectToServer(argv[1]) < 0) {
		return 3;
	}

	pthread_mutex_init(&rDataLock, NULL);
	pthread_cond_init(&dataCopyCond, NULL);

	pthread_t threads[2];
	int rc;

	if (rc = pthread_create(&threads[0], NULL, SendMsg, NULL)) {
		pthread_mutex_destroy(&rDataLock);
		pthread_cond_destroy(&dataCopyCond);
		fprintf(stderr, "Failed to create pthread\n");
		return 4;
	}
	if (rc = pthread_create(&threads[1], NULL, Render, NULL)) {
		pthread_mutex_destroy(&rDataLock);
		pthread_cond_destroy(&dataCopyCond);
		fprintf(stderr, "Failed to create pthread\n");
		return 4;
	}


	pthread_mutex_destroy(&rDataLock);
	pthread_cond_destroy(&dataCopyCond);

	RecvMsg();
	exit(0);
}
