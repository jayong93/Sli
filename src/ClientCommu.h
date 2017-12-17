#ifndef CLIENTCOMMU_H
#define CLIENTCOMMU_H
#define BUF_SIZE 100

typedef struct MsgEntry_ {
	long msgType;
	char msg[BUF_SIZE];
} MsgEntry;

void* RecvMsg();

void* SendMsg();

int ConnectToServer();

#endif
