#ifndef DATA_STRUCT_H
#define DATA_STRUCT_H
#include <pthread.h>
#include <time.h>

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

    int alive;
    int collision;
    clock_t death_time;
    
    int write_fd;
    int read_fd;
    char write_fifo[15];
    char read_fifo[15];
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

#define MAP_WIDTH 1110
#define MAP_HEIGHT 330

#endif