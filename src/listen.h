#ifndef LISTEN_H
#define LISTEN_H
#include "functions.h"


int is_exist_id( CLIENT_NODE*, char*, int);
int register_client(CLIENT_NODE**, int*, pid_t, int, char*);
int is_position_OK( CLIENT_NODE*,  AABB*);
void init_client(CLIENT*, pid_t, int, char*, int, int);

#endif