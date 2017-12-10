#define SERVER_FIFO_NAME "/tmp/.Sli/enter"
#define IPC_KEY_CON 60170
#define IPC_KEY_SND 60179
#define IPC_KEY_RCV 60178

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/msg.h>
#include <string.h>
#include <ncursesw/curses.h>
#include "ClientCommu.h"

extern pthread_mutex_t rDataLock;
extern pthread_cond_t dataCopyCond;
extern WINDOW* mainWin;
int isConnected = 0;
int channelRcv = -1;
int channelSnd = -1;
char* renderData = NULL;
int nRenderData = 0;
int maxRenderData = 0;

void SigUserHandler(int signo) {
	isConnected = 1;
}

int ConnectToServer(const char* id) {
	struct sigaction act;
	act.sa_handler = SigUserHandler;
	sigfillset(&(act.sa_mask));
	sigaction(SIGUSR1, &act, NULL);

#ifndef USE_FIFO
	int enterQueue;
	if ((enterQueue = msgget(IPC_KEY_CON, 0666|IPC_CREAT)) < 0) {
		perror("Failed to get the enter queue");
		return -1;
	}

	MsgEntry enterMsg;
	pid_t pid = getpid();
	enterMsg.msgType = pid;
	memcpy(enterMsg.msg, id, sizeof(enterMsg.msg));
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
	if ((channelRcv = open(fifoName, O_RDONLY)) < 0) {
		perror("failed to open input channel");
		return -1;
	}
	fifoName[nameLen] = 'o';
	if ((channelSnd = open(fifoName, O_WRONLY)) < 0) {
		perror("failed to open output channel");
		close(channelRcv);
		return -1;
	}
#endif
	return 0;
}

void RecvFromPipe(int* rlen, int nRead) {
	if (nRead == 0) return;

	int ret;
	if (nRead > (maxRenderData - *rlen)) {
		maxRenderData *= 2;
		renderData = (char*)realloc(renderData, maxRenderData);
	}
	// 파이프가 닫혔을 때 종료
	if (ret = read(channelRcv, renderData + *rlen, nRead) == 0)
		exit(5);
	
	*rlen += ret;
}

void RecvFromMsgQueue(int* rlen, int nRead) {
	int pid = getpid();
	if (nRead == 0) return;

	int ret;
	if (nRead > (maxRenderData - *rlen)) {
		maxRenderData *= 2;
		renderData = (char*)realloc(renderData, maxRenderData);
	}

	MsgEntry msg;
	for (int i=0; i <= ((nRead-1)/BUF_SIZE); ++i) {
		if (ret = msgrcv(channelRcv, &msg, BUF_SIZE, pid, 0) < 0)
			exit(5);
		memcpy(renderData + *rlen, &msg.msg, ret);
		*rlen += ret;
	}
}

void RecvFromServer(int* rlen, int nRead) {
#ifndef USE_FIFO
	RecvFromMsgQueue(rlen, nRead);
#else
	RecvFromPipe(rlen, nRead);
#endif
}

void* RecvMsg() {
	maxRenderData = 50;
	renderData = (char*)malloc(maxRenderData);

	// -1의 message가 왔을 때 종료 처리
	while(1) {
		int rlen = 0;
		pthread_mutex_lock(&rDataLock);
		// score
		RecvFromServer(&rlen, sizeof(int));
#ifndef USE_FIFO
		if (*(int*)renderData < 0) exit(5);
#endif
		// color
		RecvFromServer(&rlen, sizeof(int));

		int* nPoints = (int*)(renderData+rlen);
		RecvFromServer(&rlen, sizeof(int));
		for (int i=0; i<(*nPoints*2); ++i) {
			// points
			RecvFromServer(&rlen, sizeof(int));
		}

		int* nPlayers = (int*)(renderData+rlen);
		RecvFromServer(&rlen, sizeof(int));
		for (int i=0; i<*nPlayers; ++i) {
			// color
			RecvFromServer(&rlen, sizeof(int));

			int* idLen = (int*)(renderData+rlen);
			RecvFromServer(&rlen, sizeof(int));
			// id
			RecvFromServer(&rlen, *idLen);

			int* nPoints = (int*)(renderData+rlen);
			RecvFromServer(&rlen, sizeof(int));
			for (int i=0; i<*nPoints; ++i) {
				// points
				RecvFromServer(&rlen, sizeof(int)*2);
			}
		}

		int* nStars = (int*)(renderData+rlen);
		RecvFromServer(&rlen, sizeof(int));
		for (int i=0; i<*nStars; ++i) {
			RecvFromServer(&rlen, sizeof(int)*2);
		}

		// ranking
		for (int i=0; i<3; ++i) {
			int* idLen = (int*)(renderData+rlen);
			RecvFromServer(&rlen, sizeof(int));
			RecvFromServer(&rlen, *idLen);
		}

		nRenderData = rlen;

		pthread_cond_signal(&dataCopyCond);
		pthread_mutex_unlock(&rDataLock);
	}
	free(renderData);
}

void* SendMsg() {
	int pid = getpid();
	int isBoost = FALSE;
	while(1) {
		if (mainWin) {
			int ch = wgetch(mainWin);
			switch (ch) {
				case KEY_LEFT:
					ch = 'l';
					break;
				case KEY_RIGHT:
					ch = 'r';
					break;
				case KEY_UP:
					ch = 'u';
					break;
				case KEY_DOWN:
					ch = 'd';
					break;
				case ' ':
					ch = isBoost?'x':'o';
					break;
			}
#ifndef USE_FIFO
			MsgEntry msg;
			msg.msgType = pid;
			memcpy(msg.msg, &ch, sizeof(ch));
			if(msgsnd(channelSnd, &msg, sizeof(ch), 0) < 0) {
				perror("Failed to send message");
				exit(3);
			}
#else
			// TODO: PIPE 통신 구현
#endif
		}
	}
}
