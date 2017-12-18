#ifndef UPDATE_H
#define UPDATE_H
#include "listen.h"
#include <semaphore.h>


void read_clients_input(int, CLIENT_NODE**, int*, POINT_NODE**, int*, sem_t*);
void process_client_input(CLIENT*, char);
void process_out_client(CLIENT_NODE*, CLIENT_NODE**, int*, POINT_NODE**, int*, sem_t*);


void update_world(CLIENT_NODE*, int, POINT_NODE**, int*, clock_t*);
void create_star(POINT_NODE**, int*, clock_t*);
void process_dead_clients(CLIENT_NODE*, int);
void reset_client_data(CLIENT*, CLIENT_NODE*, int);
void move_clients(CLIENT_NODE*, int);
void move(CLIENT*);
void move_head(int, CLIENT*);
void move_tail(int, CLIENT*);
void collision_check(CLIENT_NODE*, int, POINT_NODE**, int*);
void collision_check_c2c(CLIENT_NODE*, int, POINT_NODE**, int*);
void collision_check_c2star(CLIENT_NODE*, int, POINT_NODE**, int*);
void collision_check_c2map(CLIENT_NODE*, int, POINT_NODE**, int*);
int is_point_in_AABB(int, int,  AABB*);
int is_crash(int, int, POINT_NODE*, int);
void process_crashed_client(CLIENT*, POINT_NODE**, int*);
int is_point_same(int, int, int, int);
void eat_star(CLIENT*);
void last_move_process(CLIENT_NODE*, int);

void send_data_to_clients(int, VECTOR*,  CLIENT_NODE*, int,  POINT_NODE*, int);
unsigned int get_send_data_size(CLIENT_NODE*, int, int );
void check_vector(VECTOR* ,unsigned int );
void fill_send_data(VECTOR*, CLIENT_NODE*, int, POINT_NODE* , int);

#endif