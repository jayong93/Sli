#include "data.h"

void create_star(time_t* , POINT** ,int* );

void delete_client(CLIENT* ,CLIENT**, POINT**, int*, int* );
void create_c2star(CLIENT*, POINT**, int*);

void add_head_point(POINT** , POINT* );
void delete_point(POINT** , POINT* );

void update_world(CLIENT**, POINT**, int*, int* );

void move_clients(CLIENT* );
void move(CLIENT* );
void move_head(int, CLIENT* );
void move_tail(int, CLIENT* );
void update_AABB(CLIENT* );
void post_move_process(CLIENT* );

void collision_check(CLIENT**, POINT**, int*, int* );
void process_collision_c2c(CLIENT**, POINT**, int*, int* );
int collision_check_c2c(CLIENT* );
int is_point_in_AABB(int, int, AABB);
int is_crash(int, int, POINT*);
void process_crashed_client(int, CLIENT**, POINT**, int*, int* );
void collision_check_c2star(CLIENT*, POINT**, int*);
int is_point_same(POINT*, POINT* );
void process_eat_star(CLIENT* );
void collision_check_c2map(CLIENT**, POINT**, int*, int* );

void send_result(VECTOR* ,CLIENT* , POINT* , int, int );
unsigned int get_send_data_size(CLIENT*, int, int );
void check_vector(VECTOR* ,unsigned int );
void fill_send_data(VECTOR*, CLIENT* , POINT* , int, int );
void send_data2clients(unsigned int, VECTOR* ,CLIENT*);
void send_data2client(unsigned int, VECTOR* ,CLIENT*);

void read_clients_input(CLIENT**, POINT**, int*, int* );
int read_client_input(CLIENT* );

int is_exist_id(CLIENT* ,char*, unsigned int);

int register_client(CLIENT**, int*, pid_t, unsigned int, char* );
void init_client(CLIENT* , pid_t, unsigned int, char* , int, int);
int is_position_OK(CLIENT* , AABB);
int is_AABB_collise(AABB, AABB);