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
#include "Util.h"

extern pthread_mutex_t rDataLock;
extern pthread_cond_t dataCopyCond;
extern WINDOW* mainWin;
int isConnected = 0;
int channelRcv = -1;
int channelSnd = -1;
VBuffer renderData;
extern const char* myID;
static pid_t pid;
static VBuffer msgBuf;	// 메시지 큐를 통해 대용량 데이터를 받기 위한 가변 버퍼

void SigUserHandler(int signo) {
	isConnected = 1;
}

int ConnectToServer() {
	struct sigaction act;
	act.sa_handler = SigUserHandler;
	sigfillset(&(act.sa_mask));
	sigaction(SIGUSR1, &act, NULL);

	pid = getpid();
	unsigned int nameLen = strlen(myID);
#ifndef USE_FIFO
	int enterQueue;
	if ((enterQueue = msgget(IPC_KEY_CON, 0666|IPC_CREAT)) < 0) {
		perror("Failed to get the enter queue");
		return -1;
	}

	MsgEntry enterMsg;
	enterMsg.msgType = pid;
	memcpy(enterMsg.msg, &nameLen, sizeof(nameLen));
	memcpy(enterMsg.msg+sizeof(nameLen), myID, nameLen);
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

	char id_buf[sizeof(pid)+sizeof(nameLen)+10];
	memcpy(id_buf, &pid, sizeof(pid));
	memcpy(id_buf+sizeof(pid), &nameLen, sizeof(nameLen));
	memcpy(id_buf+sizeof(pid)+sizeof(nameLen), myID, nameLen);
	if (write(fd, id_buf, sizeof(pid)+sizeof(nameLen)+nameLen) < 0) {
		perror("Writing failed");
		close(fd);
		return -1;
	}
	close(fd);

	do {
		pause();
	} while(!isConnected);

	char fifoName[50];
	snprintf(fifoName, sizeof(fifoName)/sizeof(*fifoName)-1, "/tmp/.Sli/%d_%s", pid, myID);
	int fifoLen = strlen(fifoName);
	fifoName[fifoLen+1] = 0;
	fifoName[fifoLen] = 'i';
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

void RecvFromPipe(VBuffer* buf, size_t nRead) {
	if (nRead == 0) return;

	int ret;
	VBAppend(buf, NULL, nRead);
	// 파이프가 닫혔을 때 종료
	if (ret = read(channelRcv, buf->ptr + buf->len, nRead) == 0)
		exit(5);
	
	buf->len += ret;
}

void RecvFromMsgQueue(VBuffer* buf, size_t nRead) {
	if (nRead == 0) return;

	int ret;
	VBAppend(&msgBuf, NULL, sizeof(long)+nRead);
	if (ret = msgrcv(channelRcv, msgBuf.ptr, nRead, pid, 0) < 0)
		exit(5);
	VBAppend(buf, msgBuf.ptr+sizeof(long), ret);
}

void RecvFromServer(VBuffer* buf, size_t nRead) {
#ifndef USE_FIFO
	RecvFromMsgQueue(buf, nRead);
#else
	RecvFromPipe(buf, nRead);
#endif
}

void* RecvMsg() {
	renderData = VBCreate(50);
	msgBuf = VBCreate(100);

	// -1의 message가 왔을 때 종료 처리
	while(1) {
		pthread_mutex_lock(&rDataLock);
		// score
		RecvFromServer(&msgBuf, sizeof(int));
#ifndef USE_FIFO
		if (*(int*)(msgBuf.ptr) < 0) exit(5);
#endif
		size_t dataLen = (size_t)(*(int*)(msgBuf.ptr));
		VBClear(&msgBuf);
		RecvFromServer(&renderData, dataLen);
		pthread_cond_signal(&dataCopyCond);
		pthread_mutex_unlock(&rDataLock);
	}
	VBDestroy(&renderData);
	VBDestroy(&msgBuf);
}

void* SendMsg() {
	int isBoost = FALSE;
	while(1) {
		if (mainWin) {
			int input = wgetch(mainWin);
			char ch;
			switch (input) {
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
			if(write(channelSnd, &ch, sizeof(ch)) < 0) {
				perror("Failed to send message");
				exit(3);
			}
#endif
		}
	}
}
