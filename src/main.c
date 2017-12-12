#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <ncursesw/curses.h>
#include "Render.h"
#include "ClientCommu.h"

pthread_mutex_t rDataLock;
pthread_cond_t dataCopyCond;
const char* myID;

int main(int argc, char* argv[]){
	if (argc < 2) {
		fprintf(stderr, "not enough arguments\n");
		return 1;
	}
	if (strlen(argv[1]) > 10) {
		fprintf(stderr, "the maximum ID length is 10\n");
		return 2;
	}
	myID = argv[1];

	if (ConnectToServer() < 0) {
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

	RecvMsg();

	pthread_mutex_destroy(&rDataLock);
	pthread_cond_destroy(&dataCopyCond);
	exit(0);
}
