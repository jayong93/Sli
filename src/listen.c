#include "listen.h"
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

int is_exist_id(CLIENT_NODE* p_head_client, char* id_buf, int len){
    CLIENT_NODE* p_cur_client = p_head_client;
    while(1){
        if(strncmp(p_cur_client->client_data.id, id_buf, len) == 0){ return 1; }
        p_cur_client = p_cur_client->next_client;
        if(p_cur_client == p_head_client) break;
    }
    return 0;
}

int register_client(CLIENT_NODE** pp_head_client, int* p_nclients, pid_t pid, int len, char* id_buf){
    int x;
    int y;

    // 생성할 위치 찾기 10번 시도
    if(*pp_head_client){
        int i = 0;
        for(i ; i < 10; ++i){
            x = (rand()%((MAP_WIDTH/10)-26)) + 10;
            y = (rand()%((MAP_HEIGHT/10)-26)) + 10;
            x = x * 10;
            y = y * 10;
            AABB aabb;
            aabb.left = x; aabb.right = x + 60; aabb.bottom = y; aabb.top = y + 60;
            if(is_position_OK(*pp_head_client, &aabb) == 1) break; 
        }
        if(i == 10) return 1;
    }
    else{
        x = (rand()%((MAP_WIDTH/10)-26)) + 10;
        y = (rand()%((MAP_HEIGHT/10)-26)) + 10;
        x = x * 10;
        y = y * 10;        
    }
    
    // client 공간 할당
    CLIENT_NODE* p_new_client = (CLIENT_NODE*)malloc(sizeof(CLIENT_NODE));
 
    // client data 채우기
    x += 30; y += 30;
    init_client(&(p_new_client->client_data), pid, len, id_buf, x, y);

    // client 연결 (무조건 head에 연결)
    if(*pp_head_client == NULL){
        p_new_client->next_client = p_new_client;
        p_new_client->prev_client = p_new_client;
    }
    else{
        p_new_client->next_client = (*pp_head_client);
        p_new_client->prev_client = (*pp_head_client)->prev_client;
        (*pp_head_client)->prev_client->next_client = p_new_client;
        (*pp_head_client)->prev_client = p_new_client;
    }
    (*pp_head_client) = p_new_client;

    *p_nclients += 1;
    return 0;
}

int is_position_OK(CLIENT_NODE* p_head_client,  AABB* p_aabb){
    CLIENT_NODE* p_cur_client = p_head_client;
    while(1){
        if(p_cur_client->client_data.alive == 1){
            if(is_AABB_collise(&(p_cur_client->client_data.collision_box), p_aabb) == 1){ return 0; }
        }
        p_cur_client = p_cur_client->next_client;
        if(p_cur_client == p_head_client) break;
    }
    return 1;
}

void init_client(CLIENT* p_client_data, pid_t pid, int len, char* id_buf, int x, int y){
    p_client_data->pid = pid;
    memcpy(p_client_data->id, id_buf, len);
    p_client_data->len_id = len;
    p_client_data->color = (rand()%64) + 1;
    p_client_data->pp_head_point = (POINT_NODE**)malloc(sizeof(POINT_NODE*));
    *(p_client_data->pp_head_point) = NULL;
    POINT_NODE* p_new_point1 = (POINT_NODE*)malloc(sizeof(POINT_NODE));
    POINT_NODE* p_new_point2 = (POINT_NODE*)malloc(sizeof(POINT_NODE));
    switch(rand()%4){
        case 0: // 가로 'l'
        {
            p_new_point1->point.x = x - 20;
            p_new_point1->point.y = y;
            p_new_point2->point.x = x + 20;
            p_new_point2->point.y = y;
            add_head_point(p_client_data->pp_head_point, p_new_point2);
            add_head_point(p_client_data->pp_head_point, p_new_point1);
            p_client_data->dir = 'l';
            p_client_data->input.dir = 'l';
            break;
        }
        case 1: // 가로 'r'
        {
            p_new_point1->point.x = x - 20;
            p_new_point1->point.y = y;
            p_new_point2->point.x = x + 20;
            p_new_point2->point.y = y;
            add_head_point(p_client_data->pp_head_point, p_new_point1);
            add_head_point(p_client_data->pp_head_point, p_new_point2);
            p_client_data->dir = 'r';
            p_client_data->input.dir = 'r';
            break;
        }
        case 2: // 세로 'd'
        {
            p_new_point1->point.x = x;
            p_new_point1->point.y = y - 20;
            p_new_point2->point.x = x;
            p_new_point2->point.y = y + 20;
            add_head_point(p_client_data->pp_head_point, p_new_point2);
            add_head_point(p_client_data->pp_head_point, p_new_point1);
            p_client_data->dir = 'd';
            p_client_data->input.dir = 'd';
            break;
        }
        case 3: // 세로 'u'
        {
            p_new_point1->point.x = x;
            p_new_point1->point.y = y - 20;
            p_new_point2->point.x = x;
            p_new_point2->point.y = y + 20;
            add_head_point(p_client_data->pp_head_point, p_new_point1);
            add_head_point(p_client_data->pp_head_point, p_new_point2);
            p_client_data->dir = 'u';
            p_client_data->input.dir = 'u';
            break;
        }
    }
    p_client_data->n_points = 2;
    p_client_data->input.boost = 'x';
    p_client_data->score = 0;
    p_client_data->alive = 1;
    p_client_data->remain_tail = 0;
    p_client_data->death_time = clock();
    p_client_data->collision = 0;
	p_client_data->use = 0;

    update_AABB(p_client_data);

    // fifo 생성 및 열기
    // fifo 이름: clientID_sc(server->client) clientID_cs(client->server)
    char read_fifo[15];
    char* r = "_cs";
    memcpy(read_fifo, id_buf, len);
    memcpy(read_fifo + len, r, 3);
    read_fifo[len + 3] = 0;
    memcpy(p_client_data->read_fifo, read_fifo, len + 3 + 1);
    if((p_client_data->read_fd = open(read_fifo, O_RDONLY)) < 0){
        printf("open read fifo error"); exit(1);
    }
    
    char write_fifo[15];
    char* w = "_sc";
    memcpy(write_fifo, id_buf, len);
    memcpy(write_fifo + len, w, 3);
    write_fifo[len + 3] = 0;
    memcpy(p_client_data->write_fifo, write_fifo, len + 3 + 1);
    if((p_client_data->write_fd = open(write_fifo, O_WRONLY)) < 0){
        printf("open write fifo error"); exit(1);
    }
    

    int flag;

    flag = fcntl(p_client_data->read_fd, F_GETFL, 0);
    flag = fcntl(p_client_data->read_fd, F_SETFL, flag | O_NONBLOCK);
    //flag = fcntl(p_client_data->write_fd, F_GETFL, 0);
    //flag = fcntl(p_client_data->write_fd, F_SETFL, flag | O_NONBLOCK);
}
