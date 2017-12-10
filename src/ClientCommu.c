#define SERVER_FIFO_NAME "/tmp/.Sli/enter"
#define IPC_KEY_CON 60170
#define IPC_KEY_SND 60179
#define IPC_KEY_RCV 60178

#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/msg.h>
#include <string.h>
#include "ClientCommu.h"

extern pthread_mutex_t rDataLock;
extern pthread_cond_t dataCopyCond;
int isConnected = 0;
int channelRcv = -1;
int channelSnd = -1;

void SigUserHandler(int signo) {
	isConnected = 1;
}

int ConnectToServer(const char* id) {
	struct sigaction act;
	act.sa_handler = SigUserHandler;
	sigfillset(&(act.sa_mask));
	sigaction(SIGUSR1, &act, NULL);

#ifdef USE_FIFO
	int enterQueue;
	if ((enterQueue = msgget(IPC_KEY_CON, 0666|IPC_CREAT)) < 0) {
		perror("Failed to get the enter queue");
		return -1;
	}

	MsgEntry enterMsg;
	pid_t pid = getpid();
	*((int*)enterMsg.msg) = pid;
	strncpy(enterMsg.msg+sizeof(int), id, sizeof(enterMsg.msg)-sizeof(int));
	if (msgsnd(enterQueue, &enterMsg, BUF_SIZE, 0) < 0) {
		perror("Failed to send a message");
		return -1;
	}

	do {
		pause();
	} while(!isConnected);

	if ((channelRcv = msgget(IPC_KEY_RCV, 0666|IPC_CREAT)) < 0) {
		perror("Failed to get the recv queue");
		return -1;
	}

	if ((channelSnd = msgget(IPC_KEY_SND, 0666|IPC_CREAT)) < 0) {
		perror("Failed to get the send queue");
		return -1;
	}

#else
	int fd;
	if ((fd = open(SERVER_FIFO_NAME, O_WRONLY)) < 0) {
		perror("A Server does not exist");
		return -1;
	}

	char id_buf[100];
	pid_t pid = getpid();
	snprintf(id_buf, sizeof(id_buf)/sizeof(*id_buf), "%d %s\n", pid, id);
	if (write(fd, id_buf, strlen(id_buf)) < 0) {
		perror("Writing failed");
		close(fd);
		return -1;
	}
	close(fd);

	do {
		pause();
	} while(!isConnected);

	char fifoName[150];
	snprintf(fifoName, sizeof(fifoName)/sizeof(*fifoName)-1, "/tmp/.Sli/%d_%s", pid, id);
	int nameLen = strlen(fifoName);
	fifoName[nameLen+1] = 0;
	fifoName[nameLen] = 'i';
	if ((channelRcv = open(fifoName, O_RDWR)) < 0) {
		perror("failed to open input channel");
		return -1;
	}
	fifoName[nameLen] = 'o';
	if ((channelSnd = open(fifoName, O_RDWR)) < 0) {
		perror("failed to open output channel");
		close(channelRcv);
		return -1;
	}
#endif
	return 0;
}

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
