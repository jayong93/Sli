#include "functions.h"
#include <math.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>


void create_star(time_t* last_time, POINT** head_star, int* n_stars){
    time_t cur_time;
    time(&cur_time);
    if(difftime(cur_time, *last_time) >= 0.5){
        int x = (rand() % MAP_WIDTH) + 10;
        int y = (rand() % MAP_HEIGHT) + 10;
        POINT* p_star = (POINT*)malloc(sizeof(POINT));
        p_star->x = x;
        p_star->y = y;
        add_head_point(head_star, p_star);
        (*n_stars) += 1;
        *last_time = cur_time;
    }
}

void delete_client(CLIENT* del_client, CLIENT** p_head_client, POINT** p_head_star, int* p_nclients, int* p_nstars){
    // client 리스트에서 del_client 제거
	printf("del client");
    del_client->prev_client->next_client = del_client->next_client;
    del_client->next_client->prev_client = del_client->prev_client;
    if(del_client == (*p_head_client)){
        if(del_client->next_client == del_client) (*p_head_client) = NULL;
        else (*p_head_client) = del_client->next_client;
    }
    (*p_nclients) -= 1;

    // client는 죽을 때 *을 남긴다
    create_c2star(del_client, p_head_star, p_nstars);

    // client에게 -1 보낸다//msg인 경우에
    ////////////////////////

    // 연결 끊는다 fd, read_fd
    close(del_client->read_fd);
    close(del_client->fd);

    // 가진 자원 전부 해제
    while(del_client->head_point) delete_point(&(del_client->head_point), del_client->head_point);
    free(del_client);
}

void create_c2star(CLIENT* del_client, POINT** p_head_star, int* p_nstars){
    POINT* cur_point = del_client->head_point;
    while(cur_point->next_point != del_client->head_point){
        POINT* next_point = cur_point->next_point;
        int x_dir = (next_point->x - cur_point->x);
        int y_dir = (next_point->y - cur_point->y);        
        if(x_dir != 0) x_dir = x_dir/abs(x_dir);
        if(y_dir != 0) y_dir = y_dir/abs(y_dir);
        int x = cur_point->x;
        int y = cur_point->y;
        while(x != next_point->x || y != next_point->y ){
            POINT* p_star = (POINT*)malloc(sizeof(POINT));
            p_star->x = x;
            p_star->y = y;
            add_head_point(p_head_star, p_star);
            (*p_nstars) += 1;
            x += x_dir * 10; y += y_dir * 10;
        }
        cur_point = next_point;
    }
    POINT* p_star = (POINT*)malloc(sizeof(POINT));
    p_star->x = cur_point->x;
    p_star->y = cur_point->y;
    add_head_point(p_head_star, p_star);
    (*p_nstars) += 1;
}

void add_head_point(POINT** head_point, POINT* new_point){
    if((*head_point) == NULL){
        new_point->next_point = new_point;
        new_point->prev_point = new_point;
    }
    else{
        new_point->next_point = (*head_point);
        new_point->prev_point = (*head_point)->prev_point;

        (*head_point)->prev_point->next_point = new_point;
        (*head_point)->prev_point = new_point;
    }
    (*head_point) = new_point;
}

void delete_point(POINT** head_point, POINT* del_point){
    del_point->prev_point->next_point = del_point->next_point;
    del_point->next_point->prev_point = del_point->prev_point;
    if(del_point == (*head_point)){
        if(del_point->next_point == del_point){
            (*head_point) = NULL;
        }
        else (*head_point) = del_point->next_point;
    }
    free(del_point);
}

void update_world(CLIENT** p_head_client, POINT** p_head_star, int* p_nclients, int* p_nstars){
    int i = 0;

    for(; i < 2; i += 1){
        move_clients(*p_head_client);
        
        collision_check(p_head_client, p_head_star, p_nclients, p_nstars);
    }
    post_move_process(*p_head_client);
}

void move_clients(CLIENT* head_client){
    CLIENT* cur_client = head_client;
    while(1){
        move(cur_client);

        cur_client = cur_client->next_client;
        if(cur_client == head_client) break;
    }
}

void post_move_process(CLIENT* head_client){
    CLIENT* cur_client = head_client;
    while(1){
        if(cur_client->input.boost == 'o'){
            cur_client->score -= 1;
            if(cur_client->remain_tail <= 0) move_tail(10, cur_client);
        }
        if(cur_client->remain_tail > 0) cur_client->remain_tail -= 1;

        cur_client = cur_client->next_client;
        if(cur_client == head_client) break;
    }
}

void move(CLIENT* client){
    // boost 가능 여부 확인
    if((client->input.boost == 'o') && (client->score <= 0)) client->input.boost = 'x';

    int head_speed = 5;
    int tail_speed = 5;

    if(client->remain_tail > 0){
        tail_speed = 0;
    }
    if(client->input.boost == 'o'){
        head_speed = 10; tail_speed = 10;
    }

    move_head(head_speed, client);
    move_tail(tail_speed, client);
    update_AABB(client);
}

void move_head(int speed, CLIENT* client){
	printf("in move_head\n");
    int x_dir = 0;
    int y_dir = 0;
    char dir = client->input.dir;
    switch(dir){
        case 'u':{ y_dir=1;break;}
        case 'd': {y_dir=-1;break;}
        case 'r': {x_dir=1;break;}
        case 'l': {x_dir=-1;break;}
    }
    if((client->dir) == dir){
        client->head_point->x += x_dir*speed;
		printf("%d\n", client->head_point->x);
        client->head_point->y += y_dir*speed;
		printf("%d\n", client->head_point->y);
    }
    else{
        POINT* p_point = (POINT*)malloc(sizeof(POINT));
        p_point->x = client->head_point->x + (x_dir*speed);
        p_point->y = client->head_point->y + (y_dir*speed);
        add_head_point(&(client->head_point), p_point);
        client->n_points += 1;
        client->dir = dir;
    }
}

void move_tail(int speed, CLIENT* client){
    POINT* tail_point = client->head_point->prev_point;
    POINT* pre_tail_point = tail_point->prev_point;
    int x_dir = (pre_tail_point->x - tail_point->x);
    int y_dir = (pre_tail_point->y - tail_point->y);
    if(x_dir != 0) x_dir = x_dir/abs(x_dir);
    if(y_dir != 0) y_dir = y_dir/abs(y_dir);
    tail_point->x += x_dir*speed;
    tail_point->y += y_dir*speed;
    if((tail_point->x == pre_tail_point->x) && (tail_point->y == pre_tail_point->y)){
        delete_point(&(client->head_point), tail_point);
        client->n_points -= 1;
    } 
}

void update_AABB(CLIENT* client){
    int left = MAP_WIDTH; int right = 0; int top = 0; int bottom = MAP_HEIGHT;
    POINT* cur_point = client->head_point;
    while(1){
        if(cur_point->x < left) left = cur_point->x;
        if(cur_point->x > right) right = cur_point->x;
        if(cur_point->y < bottom) bottom = cur_point->y;
        if(cur_point->y > top) top = cur_point->y;

        cur_point = cur_point->next_point;
        if(cur_point == client->head_point) break;
    }
    left -= 10; right += 10; bottom -= 10; top += 10;
    client->collision_box.left = left;
    client->collision_box.right = right;
    client->collision_box.bottom = bottom;
    client->collision_box.top = top;
}

void collision_check(CLIENT** p_head_client, POINT** p_head_star, int* p_nclients, int* p_nstars){
    if(*p_nclients > 1) process_collision_c2c(p_head_client, p_head_star, p_nclients, p_nstars);
    if(*p_nclients > 0 && *p_nstars > 0) collision_check_c2star(*p_head_client, p_head_star, p_nstars);
    if(*p_nclients > 0) collision_check_c2map(p_head_client, p_head_star, p_nclients, p_nstars);
}

void process_collision_c2c(CLIENT** p_head_client, POINT** p_head_star, int* p_nclients, int* p_nstars){
    // client 끼리 서로 충돌 체크
    int n_collision = collision_check_c2c(*p_head_client);
    // 죽은 client 처리
    if(n_collision) process_crashed_client(n_collision, p_head_client, p_head_star, p_nclients, p_nstars);
}

int collision_check_c2c(CLIENT* head_client){
    int n_collision = 0;
    CLIENT* cur_client = head_client;
    while(1){
        CLIENT* counter_client = cur_client->next_client;
        if(counter_client == cur_client) break;

        while(1){
            if(is_point_in_AABB(cur_client->head_point->x, cur_client->head_point->y, counter_client->collision_box)){
                if(is_crash(cur_client->head_point->x, cur_client->head_point->y, counter_client->head_point)){
                    cur_client->alive = 0;
                    n_collision += 1;
                    break;
                }
            }
            counter_client = counter_client->next_client;
            if(counter_client == cur_client) break;
        }
 
        cur_client = cur_client->next_client;
        if(cur_client == head_client) break;
    }
    return n_collision;
}

int is_point_in_AABB(int x, int y, AABB aabb){
    if((aabb.left <= x && x <= aabb.right) && (aabb.bottom <= y && y <= aabb.top))return 1;
    return 0;
}

int is_crash(int x, int y, POINT* head_point){
    POINT* cur_point = head_point;
    while(1){
        POINT* p1 = cur_point;
        POINT* p2 = cur_point->next_point;
        int left = (p1->x <= p2->x)?p1->x:p2->x;
        int right = (p1->x >= p2->x)?p1->x:p2->x;
        int bottom = (p1->y <= p2->y)?p1->y:p2->y;
        int top = (p1->y >= p2->y)?p1->y:p2->y;
        if((left <= x && x <= right) && (bottom <= y && y <= top)){
            return 1;
        }
        cur_point = cur_point->next_point;
        if(cur_point->next_point == head_point) break;
    }
    return 0;
}

void process_crashed_client(int n_collision, CLIENT** p_head_client, POINT** p_head_star, int* p_nclients, int* p_nstars){
    int n_processed = 0;
    CLIENT* cur_client = *p_head_client;
    while(n_processed != n_collision){
        if(cur_client->alive == 0){
            CLIENT* del_client = cur_client;
            cur_client = del_client->next_client;
            delete_client(del_client, p_head_client, p_head_star, p_nclients, p_nstars);
            n_processed += 1;
            continue;
        }
        cur_client = cur_client->next_client;
    }
}

void collision_check_c2star(CLIENT* head_client, POINT** p_head_star, int* p_nstars){
    CLIENT* cur_client = head_client;
    while(1){
        POINT* cur_star = *p_head_star;
        while(1){
            if(is_point_same(cur_client->head_point, cur_star)){
                int is_tail_star = 0;
                // client data 수정
                process_eat_star(cur_client);
                // 충돌한 * 삭제
                POINT* del_star = cur_star;
                if(del_star == (*p_head_star)->prev_point) is_tail_star = 1;
                cur_star = cur_star->next_point;
                delete_point(p_head_star, del_star);
                *p_nstars -= 1;
                if(*p_nstars == 0) return;
                if(is_tail_star) break;
                continue;
            }
            cur_star = cur_star->next_point;
            if(cur_star == (*p_head_star)) break;
        }

        cur_client = cur_client->next_client;
        if(cur_client == head_client) break;
    }
}

int is_point_same(POINT* p1, POINT* p2){
    if((p1->x == p2->x) && (p1->y == p2->y)) return 1;
    return 0;
}

void process_eat_star(CLIENT* client){
    client->score += 1;
    client->remain_tail += 1;
}

void collision_check_c2map(CLIENT** p_head_client, POINT** p_head_star, int* p_nclients, int* p_nstars){
    // head가 map 밖에 나갔는지 검사
    AABB map;
    map.left = 0; map.right = MAP_WIDTH; map.bottom = 0; map.top = MAP_HEIGHT;
    CLIENT* cur_client = *p_head_client;
    int n_collision = 0;
    while(1){
        if(is_point_in_AABB(cur_client->head_point->x, cur_client->head_point->y, map) == 0){
            cur_client->alive = 0;
            n_collision += 1;
        }
        cur_client = cur_client->next_client;
        if(cur_client == *p_head_client) break;
    }
    // 충돌한 client 처리
    if(n_collision) process_crashed_client(n_collision, p_head_client, p_head_star, p_nclients, p_nstars);
}



void send_result(VECTOR* p_vector, CLIENT* head_client, POINT* head_star, int n_clients, int n_stars){
    unsigned int data_size = 0;
    data_size = get_send_data_size(head_client, n_clients, n_stars);
    check_vector(p_vector, data_size);
    fill_send_data(p_vector, head_client, head_star, n_clients, n_stars);

    send_data2clients(data_size, p_vector, head_client);
}

unsigned int get_send_data_size(CLIENT* head_client, int n_clients, int n_stars){
    // nUser + (score + color + lenID + id + nPoints + points) + (...) + (...) 
    //  + nStar + nStar*(x + y)
    int i = 0;
    unsigned int data_size = 0;
    
    data_size += sizeof(int);
    CLIENT* cur_client = head_client;
    for(i; i < n_clients; ++i){
        data_size += sizeof(int);
        data_size += sizeof(unsigned char);
        data_size += sizeof(unsigned int);
        data_size += sizeof(char) * (cur_client->len_id);
        data_size += sizeof(unsigned int);
        data_size += sizeof(int) * 2 * (cur_client->n_points);
        cur_client = cur_client->next_client;
    }
    
    data_size += sizeof(int);
    data_size += sizeof(int) * 2 * n_stars;

    return data_size;
}

void check_vector(VECTOR* vector, unsigned int need){
    if(vector->capacity < need){
        if(vector->p_data) free(vector->p_data);
        vector->p_data = (char*)malloc(need*2);
        vector->capacity = need*2;
    }
}

void fill_send_data(VECTOR* p_vector, CLIENT* head_client, POINT* head_star, int n_clients, int n_stars){
    char* p_data = p_vector->p_data;
    // nUser + (score + color + lenID + id + nPoints + points) + (...) + (...) 
    //  + nStar + nStar*(x + y)
    memcpy(p_data, &n_clients, sizeof(int));
    p_data += sizeof(int);
    
    int i = 0;
    int j = 0;
    CLIENT* cur_client = head_client;
    for( i; i < n_clients; ++i){
        memcpy(p_data ,&(cur_client->score), sizeof(int));
        p_data += sizeof(int);
        memcpy(p_data ,&(cur_client->color), sizeof(unsigned char));
        p_data += sizeof(unsigned char);
        memcpy(p_data ,&(cur_client->len_id), sizeof(unsigned int));
        p_data += sizeof(unsigned int);
        memcpy(p_data ,&(cur_client->id), cur_client->len_id);  //null은 copy하지 않는다
        p_data += cur_client->len_id;
        memcpy(p_data ,&(cur_client->n_points), sizeof(unsigned int));
        p_data += sizeof(unsigned int);

        POINT* cur_point = cur_client->head_point;
        for(j = 0; j < cur_client->n_points; ++j){
            memcpy(p_data, &(cur_point->x), sizeof(int));
            p_data += sizeof(int);
            memcpy(p_data, &(cur_point->y), sizeof(int));
            p_data += sizeof(int);
            
            cur_point = cur_point->next_point;
        }

        cur_client = cur_client->next_client;
    }

    memcpy(p_data, &n_stars, sizeof(int));
    p_data += sizeof(int);
    POINT* cur_star = head_star;

    for(j = 0; j < n_stars; ++j){
        memcpy(p_data, &(cur_star->x), sizeof(int));
        p_data += sizeof(int);
        memcpy(p_data, &(cur_star->y), sizeof(int));
        p_data += sizeof(int);
        
        cur_star = cur_star->next_point;
    }
}

void send_data2clients(unsigned int data_size, VECTOR* p_vector, CLIENT* head_client){
    CLIENT* cur_client = head_client;
    while(1){
        send_data2client(data_size, p_vector, cur_client);

        cur_client = cur_client->next_client;
        if(cur_client == head_client) break;
    }
}


void send_data2client(unsigned int data_size, VECTOR* p_vector, CLIENT* client){
	printf("dsize %d\n", data_size);
    if(write(client->fd, &data_size, sizeof(unsigned int)) == -1) {
        printf("message write failed"); exit(1);
        }
    if(write(client->fd, p_vector->p_data, data_size) == -1) {
        printf("message write failed"); exit(1);
        }
}

void read_clients_input(CLIENT** p_head_client, POINT** p_head_star, int* p_nclients, int* p_nstars){ 
    // client가 보낸 데이터 읽기
    // 이때 클라이언트가 나갔는지도 검사 read_client_input 이 -1을 리턴하면 나간 것
    CLIENT* cur_client = *p_head_client;
    int n_out = 0;
    while(1){
        if(read_client_input(cur_client) == -1){
            cur_client->alive = 0;
            n_out += 1;
        }
        cur_client = cur_client->next_client;
        if(cur_client == *p_head_client)break;
    }

    // 나간 클라이언트 처리
    if(n_out) process_crashed_client(n_out, p_head_client, p_head_star, p_nclients, p_nstars);
}

int read_client_input(CLIENT* client){
    int ret = 0;
    char buf;
    int nread;
    // 비동기 io
    switch(nread = read(client->read_fd, &buf, sizeof(char))){
        case -1:
        {
            if(errno == EAGAIN) break;
            else{
                printf("message read failed"); exit(1);
            }
        }
        case 0:     //파이프가 닫혔다
        {
            // 해당 클라이언트 죽인다
            ret = -1;
            break;
        }
        default:{
            // update client data
            switch(buf){
                case 'l':
                case 'r':
                case 'd':
                case 'u':
                {
                    client->input.dir = buf;
                    break;
                }
                case 'o':
                case 'x':
                {
                    client->input.boost = buf;
                    break;
                }
                default:
                {
                    break;
                }
            }
            break;
        }
    }
    return ret;
}

int is_exist_id(CLIENT* head_client, char* id_buf, unsigned int len){
    CLIENT* cur_client = head_client;
    while(1){
        if(strncmp(cur_client->id, id_buf, len) == 0){
            return 1;
        }
        cur_client = cur_client->next_client;
        if(cur_client == head_client) break;
    }
    return 0;
}

//등록 못하면 1 리턴
int register_client(CLIENT** p_head_client, int* p_nclients, pid_t pid, unsigned int len, char* id_buf){
    int x;
    int y;
    // 위치 찾기 10번 시도
    if(*p_head_client){
        int i = 0;
        for(i ; i < 10; ++i){
            x = (rand()%((MAP_WIDTH/10)-26)) + 10;
            y = (rand()%((MAP_HEIGHT/10)-26)) + 10;
            x = x * 10;
            y = y * 10;
            AABB aabb;
            aabb.left = x; aabb.right = x + 60; aabb.bottom = y; aabb.top = y + 60;
            if(is_position_OK(*p_head_client, aabb) == 1) break; 
        }
        if(i == 0) return 1;
    }
    else{
        x = (rand()%((MAP_WIDTH/10)-26)) + 10;
        y = (rand()%((MAP_HEIGHT/10)-26)) + 10;
        x = x * 10;
        y = y * 10;        
    }
    
    // client 공간 할당
    CLIENT* new_client = (CLIENT*)malloc(sizeof(CLIENT));
    
    // client data 채우기
    x += 30; y += 30;
    init_client(new_client, pid, len, id_buf, x, y);

    // client 연결 (무조건 head에 연결)
    if(*p_head_client == NULL){
        new_client->next_client = new_client;
        new_client->prev_client = new_client;
    }
    else{
        new_client->next_client = (*p_head_client);
        new_client->prev_client = (*p_head_client)->prev_client;

        (*p_head_client)->prev_client->next_client = new_client;
        (*p_head_client)->prev_client = new_client;
    }
    (*p_head_client) = new_client;

    *p_nclients += 1;
    return 0;
}

void init_client(CLIENT* new_client, pid_t pid, unsigned int len, char* id_buf, int x, int y){
    new_client->pid = pid;
    memcpy(new_client->id, id_buf, len);
    new_client->len_id = len;
    new_client->color = (rand()%64) + 1;
    new_client->head_point = NULL;
    switch(rand()%4){
        case 0: //가로 'l'
        {
            POINT* new_point1 = (POINT*)malloc(sizeof(POINT));
            new_point1->x = x - 20;
            new_point1->y = y;
            POINT* new_point2 = (POINT*)malloc(sizeof(POINT));
            new_point2->x = x + 20;
            new_point2->y = y;
            add_head_point(&(new_client->head_point), new_point2);
            add_head_point(&(new_client->head_point), new_point1);
            new_client->dir = 'l';
            new_client->input.dir = 'l';
            break;
        }
        case 1: //가로 r
        {
            POINT* new_point1 = (POINT*)malloc(sizeof(POINT));
            new_point1->x = x - 20;
            new_point1->y = y;
            POINT* new_point2 = (POINT*)malloc(sizeof(POINT));
            new_point2->x = x + 20;
            new_point2->y = y;
            add_head_point(&(new_client->head_point), new_point1);
            add_head_point(&(new_client->head_point), new_point2);
            new_client->dir = 'r';
            new_client->input.dir = 'r';
            break;
        }
        case 2: //세로 d
        {
            POINT* new_point1 = (POINT*)malloc(sizeof(POINT));
            new_point1->x = x;
            new_point1->y = y - 20;
            POINT* new_point2 = (POINT*)malloc(sizeof(POINT));
            new_point2->x = x;
            new_point2->y = y + 20;
            add_head_point(&(new_client->head_point), new_point2);
            add_head_point(&(new_client->head_point), new_point1);
            new_client->dir = 'd';
            new_client->input.dir = 'd';
            break;
        }
        case 3: //세로 u
        {
            POINT* new_point1 = (POINT*)malloc(sizeof(POINT));
            new_point1->x = x;
            new_point1->y = y - 20;
            POINT* new_point2 = (POINT*)malloc(sizeof(POINT));
            new_point2->x = x;
            new_point2->y = y + 20;
            add_head_point(&(new_client->head_point), new_point1);
            add_head_point(&(new_client->head_point), new_point2);
            new_client->dir = 'u';
            new_client->input.dir = 'u';
            break;
        }
    }
    new_client->n_points = 2;
    new_client->input.boost = 'x';
    new_client->score = 0;
    new_client->alive = 1;
    new_client->remain_tail = 0;
    update_AABB(new_client);
    
    //////////fifo 생성 및 열기
    // fifo 이름: clientID_sc(server->client) clientID_cs(client->server)
    char read_fifo[15];
    char* r = "_cs";
    memcpy(read_fifo, id_buf, len);
    memcpy(read_fifo + len, r, 3);
    read_fifo[len + 3] = 0;
     memcpy(new_client->read_fifo, read_fifo, len + 3 + 1);

    char write_fifo[15];
    char* w = "_sc";
    memcpy(write_fifo, id_buf, len);
    memcpy(write_fifo + len, w, 3);
    write_fifo[len + 3] = 0;
    memcpy(new_client->write_fifo, write_fifo, len + 3 + 1);
    
    // make fifo
    //if(mkfifo(read_fifo, 0666) == -1){
        //if(errno != EEXIST){
            //printf("make fifo error"); exit(1);
        //}
    //}
	/*
    if(mkfifo(write_fifo, 0666) == -1){
        if(errno != EEXIST){
            printf("fifo error"); exit(1);
        }
    }
	*/

    // open fifo
	printf("oepn %s\n", read_fifo);
    if((new_client->read_fd = open(read_fifo, O_RDONLY| O_NONBLOCK)) < 0){
        printf("open read fifo error"); exit(1);
    }
	printf("open read fifo %d\n", new_client->read_fd);
	printf("oepn %s\n", write_fifo);
    if((new_client->fd = open(write_fifo, O_WRONLY|O_NONBLOCK)) < 0){
        printf("open write fifo error"); exit(1);
    }
	printf("open write fifo %d\n", new_client->fd);

	sleep(2);
}

int is_position_OK(CLIENT* head_client, AABB aabb){
    CLIENT* cur_client = head_client;
    while(1){
        if(is_AABB_collise(cur_client->collision_box, aabb) == 1){
            return 0;
        }
        cur_client = cur_client->next_client;
        if(cur_client == head_client) break;
    }
    return 1;
}

int is_AABB_collise(AABB aabb1, AABB aabb2){
    if(((aabb1.left < aabb2.right) && (aabb2.left < aabb1.right))
        && ((aabb1.bottom < aabb2.top) && (aabb2.bottom < aabb1.top))){
            return 1;
        }
    return 0;
}
