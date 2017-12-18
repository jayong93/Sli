#ifndef DATA_STRUCT_H
#define DATA_STRUCT_H
#include <pthread.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/msg.h>

typedef struct POINT_{
    int x;
    int y;
}POINT;

typedef struct POINT_NODE_{
    POINT point;

    struct POINT_NODE_* prev_point;
    struct POINT_NODE_* next_point;
}POINT_NODE;

typedef struct INPUT_{
    char dir;
    char boost;
}INPUT;

typedef struct AABB_{
    int top;
    int bottom;
    int left;
    int right;
}AABB;


typedef struct CLIENT_{
    pid_t pid;
    char id[11];
    unsigned int len_id;
    unsigned char color;
    POINT_NODE** pp_head_point;      //double linked list;
    unsigned int n_points;
    INPUT input;
    char dir;
    AABB collision_box;
    int score;
    int remain_tail;
	int use;

    int alive;
    int collision;
    clock_t death_time;
    
}CLIENT;

typedef struct CLIENT_NODE_{
    CLIENT client_data;
    
    struct CLIENT_NODE_* prev_client;
    struct CLIENT_NODE_* next_client;
}CLIENT_NODE;

typedef struct VECTOR_{
    char* p_data;

    unsigned int data_size;
    unsigned int capacity;
}VECTOR;

#define BUF_SIZE 256

typedef struct MSG_{
    long mtype;
    char mdata[BUF_SIZE];
}MSG;

#define LISTEN_QKEY (key_t)60170
#define CS_QKEY     (key_t)60174
#define SC_QKEY     (key_t)60179

#define MAP_WIDTH 1110
#define MAP_HEIGHT 330

#endif
