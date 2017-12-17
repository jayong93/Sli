#define SERVER_FIFO_NAME "listen_fifo"
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
#include <sys/types.h>
#include <sys/stat.h>
#include <ncurses.h>
#include "ClientCommu.h"
#include "Util.h"

extern pthread_mutex_t rDataLock;
extern pthread_mutex_t inputLock;
extern pthread_cond_t inputCond;
extern WINDOW* mainWin;
int isConnected = 0;
int channelRcv = -1;
int channelSnd = -1;
VBuffer renderData;
int isUpdated = 0;
int isNameEnabled = 0;
extern const char* myID;
static pid_t pid;
static VBuffer msgBuf;	// 메시지 큐를 통해 대용량 데이터를 받기 위한 가변 버퍼

void SigUserHandler(int signo) {
	isConnected = 1;
}

int ConnectToServer() {
	//struct sigaction act;
	//act.sa_handler = SigUserHandler;
	//sigfillset(&(act.sa_mask));
	//sigaction(SIGUSR1, &act, NULL);
	signal(SIGUSR1, SigUserHandler);

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

	printf("Waiting Signal\n");
	do {
		pause();
	} while(!isConnected);
	printf("Signal Received\n");

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
	char inPipeName[50];
	char outPipeName[50];
	snprintf(inPipeName, sizeof(inPipeName)/sizeof(*inPipeName)-1, "%s_sc", myID);
	snprintf(outPipeName, sizeof(outPipeName)/sizeof(*outPipeName)-1, "%s_cs", myID);
	if ((fd = open(SERVER_FIFO_NAME, O_WRONLY)) < 0) {
		perror("A Server does not exist");
		return -1;
	}

	if (mkfifo(inPipeName, 0666) < 0) {
		perror("Failed to make In FIFO");
		return -1;
	}
	if (mkfifo(outPipeName, 0666) < 0) {
		perror("Failed to make In FIFO");
		return -1;
	}

	sigset_t blockSet;
	sigfillset(&blockSet);
	sigprocmask(SIG_SETMASK, &blockSet, NULL);

	char id_buf[sizeof(pid)+sizeof(nameLen)+10];
	memcpy(id_buf, &pid, sizeof(pid));
	memcpy(id_buf+sizeof(pid), &nameLen, sizeof(nameLen));
	memcpy(id_buf+sizeof(pid)+sizeof(nameLen), myID, nameLen);
	
	int dataLen = sizeof(pid)+sizeof(nameLen)+nameLen;
	struct flock lockType;
	lockType.l_type = F_WRLCK;
	lockType.l_whence = SEEK_END;
	lockType.l_start = 0;
	lockType.l_len = dataLen;

	if (fcntl(fd, F_SETLKW, &lockType) < 0) {
		perror("Failed to file locking");
		return -1;
	}
	if (write(fd, id_buf, dataLen) < 0) {
		perror("Writing failed");
		close(fd);
		return -1;
	}
	lockType.l_type = F_UNLCK;
	if (fcntl(fd, F_SETLK, &lockType) < 0) {
		perror("Failed to file unlocking");
		return -1;
	}
	close(fd);

	printf("open %s\n", outPipeName);
	if ((channelSnd = open(outPipeName, O_WRONLY)) < 0) {
		perror("failed to open output channel");
		close(channelRcv);
		return -1;
	}

	printf("open %s\n", inPipeName);
	if ((channelRcv = open(inPipeName, O_RDONLY)) < 0) {
		perror("failed to open input channel");
		return -1;
	}

	sigprocmask(SIG_UNBLOCK, &blockSet, NULL);

	printf("Waiting Signal\n");
	while(isConnected == 0) {
		pause();
		printf("Some Signal Received\n");
	}
	printf("Signal Received\n");

	printf("After init\n");
#endif
	return 0;
}

void RecvFromPipe(VBuffer* buf, size_t nRead) {
	if (nRead == 0) return;

	int ret;
	VBAppend(buf, NULL, nRead);
	// 파이프가 닫혔을 때 종료
	if ((ret = read(channelRcv, buf->ptr, nRead)) < 0) {
		endwin();
		perror("Fail to read from pipe");
		exit(5);
	}
	else if (ret == 0) {
		endwin();
		fprintf(stderr, "pipe is closed\n");
		exit(6);
	}
	
	buf->len = ret;
}

void RecvFromMsgQueue(VBuffer* buf, size_t nRead) {
	if (nRead == 0) return;

	int ret;
	VBClear(&msgBuf);
	VBAppend(&msgBuf, NULL, sizeof(long)+nRead);
	if (ret = msgrcv(channelRcv, msgBuf.ptr, nRead, pid, 0) < 0) {
		endwin();
		perror("Fail to read from MsgQueue");
		exit(5);
	}
	VBReplace(buf, msgBuf.ptr+sizeof(long), ret);
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

	while(1) {
		RecvFromServer(&msgBuf, sizeof(int));
#ifndef USE_FIFO
		if (*(unsigned int*)(msgBuf.ptr) < 0) {endwin(); fprintf(stderr, "bad data\n"); exit(5);}
#endif
		size_t dataLen = (size_t)(*(unsigned int*)(msgBuf.ptr));
		pthread_mutex_lock(&rDataLock);
		RecvFromServer(&renderData, dataLen);
		isUpdated = 1;
		pthread_mutex_unlock(&rDataLock);
	}
	VBDestroy(&renderData);
	VBDestroy(&msgBuf);
}

int GetInput(WINDOW* win, void* data) {
	return wgetch(win);
}

void* SendMsg() {
	int isBoost = FALSE;
	pthread_mutex_lock(&inputLock);
	pthread_cond_wait(&inputCond, &inputLock);
	pthread_mutex_unlock(&inputLock);
	while(1) {
		if (mainWin) {
			int input = use_window(mainWin, GetInput, NULL);
			if(input == ERR) continue;
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
					isBoost = !isBoost;
					break;
				case 'z':
					pthread_mutex_lock(&inputLock);
					isNameEnabled = !isNameEnabled;
					pthread_mutex_unlock(&inputLock);
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
			if(write(channelSnd, &ch, sizeof(ch)) < 0) {
				perror("Failed to send message");
				exit(3);
			}
#endif
		}
	}
}
