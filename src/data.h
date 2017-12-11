#include <fcntl.h>
#include <pthread.h>

typedef struct POINT_{
    int x;
    int y;
    struct POINT_* prev_point;
    struct POINT_* next_point;
}POINT;

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
    POINT* head_point;      //double linked list;
    unsigned int n_points;
    INPUT input;
    char dir;
    AABB collision_box;
    int score;
    int alive;
    int remain_tail;

    char write_fifo[15];
    int fd;
    char read_fifo[15];
    int read_fd;

    struct CLIENT_* prev_client;
    struct CLIENT_* next_client;
}CLIENT;

typedef struct VECTOR_{
    char* p_data;
    unsigned int capacity;
}VECTOR;

#define MAP_WIDTH 1110
#define MAP_HEIGHT 330