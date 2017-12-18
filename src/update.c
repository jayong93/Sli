#include "update.h"
#include <math.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>

void read_clients_input(CLIENT_NODE** pp_head_client, int* p_nclients, POINT_NODE** pp_head_star, int* p_nstars, sem_t* p_sem){
    // 클라이언트들로 부터 받은 입력 읽기 + 나간 클라이언트 처리
    CLIENT_NODE* p_cur_client = *pp_head_client;
    int nread;
    char buf;
    while(1){
        // 비동기 IO
        nread = read(p_cur_client->client_data.read_fd, &buf, sizeof(char));
        if(nread < 0){
            if(errno != EAGAIN){
                printf("message read failed"); exit(1);
            }
        }
        else if(nread == 0){
            // 클라이언트가 나감
            printf("client out\n"); 
			int out = 0;
            CLIENT_NODE* p_del_client = p_cur_client;
            p_cur_client = p_cur_client->next_client;
            if(p_cur_client == *pp_head_client) out = 1;

            process_out_client(p_del_client, pp_head_client, p_nclients, pp_head_star, p_nstars, p_sem);
            
            if(out == 1) break;
            continue;
        }
        else{
            process_client_input(&(p_cur_client->client_data), buf);
        }

        p_cur_client = p_cur_client->next_client;
        if(p_cur_client == *pp_head_client) break;
    }
}

void process_client_input(CLIENT* p_client_data, char input){
    switch(input){
        case 'l':
        case 'r':
        case 'd':
        case 'u':
        {
            p_client_data->input.dir = input;
            break;
        }
        case 'o':
        case 'x':
        {
            p_client_data->input.boost = input;
            break;
        }
        default:
        {
            break;
        }
    }
}

void process_out_client(CLIENT_NODE* p_del_client, CLIENT_NODE** pp_head_client, int* p_nclients, 
                            POINT_NODE** pp_head_star, int* p_nstars, sem_t* p_sem)
{
    // client 리스트에서 p_del_client 제거
    p_del_client->next_client->prev_client = p_del_client->prev_client;
    p_del_client->prev_client->next_client = p_del_client->next_client;
    if(p_del_client == (*pp_head_client)){
        if(p_del_client->next_client == p_del_client) *pp_head_client = NULL;
        else *pp_head_client = p_del_client->next_client;
    }
    *p_nclients -= 1;

    // client는 죽을 때 *을 남긴다
    create_points2star(*(p_del_client->client_data.pp_head_point), pp_head_star, p_nstars);

    // 연결 끊는다 fd, read_fd + fifo 삭제
    if(close(p_del_client->client_data.read_fd) == -1){
        printf("close error \n"); exit(1);
    }
    if(close(p_del_client->client_data.write_fd) == -1){
        printf("close error \n"); exit(1);
    }
    if(unlink(p_del_client->client_data.read_fifo) == -1){
        printf("unlink error \n"); exit(1);
    }
    if(unlink(p_del_client->client_data.write_fifo) == -1){
        printf("unlink error \n"); exit(1);
    }

    // 가진 자원 전부 해제
    while(*(p_del_client->client_data.pp_head_point) != NULL){
        delete_point(*(p_del_client->client_data.pp_head_point), p_del_client->client_data.pp_head_point);
    }
    free(p_del_client->client_data.pp_head_point);
    free(p_del_client);
	printf("sem wait\n");
    sem_wait(p_sem);
}


void update_world(CLIENT_NODE* p_head_client, int n_clients, 
                    POINT_NODE** pp_head_star, int* p_nstars, clock_t* p_last_star_time)
{
	create_star(pp_head_star, p_nstars, p_last_star_time);
	process_dead_clients(p_head_client, n_clients);

    int i = 0;
    for(i; i < 2; ++i){
        move_clients(p_head_client, n_clients);
        collision_check(p_head_client, n_clients, pp_head_star, p_nstars);
    }
    last_move_process(p_head_client, n_clients);

}


void last_move_process(CLIENT_NODE* p_head_client, int n_clients){
    CLIENT_NODE* p_cur_client = p_head_client;
    int i = 0;
    for(i; i < n_clients; ++i){
        if(p_cur_client->client_data.alive == 1){
			if(p_cur_client->client_data.input.boost == 'o'){
				p_cur_client->client_data.score -= 1;
				if(p_cur_client->client_data.remain_tail <= 0) move_tail(10, &(p_cur_client->client_data));
			}
			if(p_cur_client->client_data.use == 1){
				p_cur_client->client_data.use = 0;
				p_cur_client->client_data.remain_tail -= 1;
			}
		}	
        p_cur_client = p_cur_client->next_client;
    }
}

void collision_check(CLIENT_NODE* p_head_client, int n_clients, POINT_NODE** pp_head_star, int* p_nstars){
	if(n_clients > 1) collision_check_c2c(p_head_client, n_clients, pp_head_star, p_nstars);
    if(*p_nstars > 0) collision_check_c2star(p_head_client, n_clients, pp_head_star, p_nstars);
    //collision_check_c2map(p_head_client, n_clients, pp_head_star, p_nstars);
}

void collision_check_c2map(CLIENT_NODE* p_head_client, int n_clients, POINT_NODE** pp_head_star, int* p_nstars){
    // head가 map 밖에 나갔는지 검사
    AABB map;
    map.left = 0; map.right = MAP_WIDTH; map.bottom = 0; map.top = MAP_HEIGHT;
    
    CLIENT_NODE* p_cur_client = p_head_client;
    int i = 0;
    for(i; i < n_clients; ++i){
        if(p_cur_client->client_data.alive == 1){
            if(is_point_in_AABB((*(p_cur_client->client_data.pp_head_point))->point.x
                                , (*(p_cur_client->client_data.pp_head_point))->point.y, &map) == 0){
                process_crashed_client(&(p_cur_client->client_data), pp_head_star, p_nstars);
            }
        }  
        p_cur_client = p_cur_client->next_client;
    }
}

void collision_check_c2star(CLIENT_NODE* p_head_client, int n_clients, POINT_NODE** pp_head_star, int* p_nstars){
    CLIENT_NODE* p_cur_client = p_head_client;
    int i = 0;
    for(i; i < n_clients; ++i){
        if(p_cur_client->client_data.alive == 1){
            POINT_NODE* p_cur_star = *pp_head_star;
            while(1){
                if(is_point_same((*(p_cur_client->client_data.pp_head_point))->point.x 
                                 ,(*(p_cur_client->client_data.pp_head_point))->point.y
                                 , p_cur_star->point.x, p_cur_star->point.y))
                {
                    int is_tail_star = 0;
                    POINT_NODE* p_del_star = p_cur_star;
                    p_cur_star = p_cur_star->next_point;
                    if(p_cur_star == *pp_head_star) is_tail_star = 1;
                    delete_point(p_del_star, pp_head_star);
                    *p_nstars -= 1;
                    eat_star(&(p_cur_client->client_data));
                    // 별이 하나도 안남으면 return
                    if(*p_nstars <= 0) return;
                    if(is_tail_star == 1) break;
                    continue;
                }
                p_cur_star = p_cur_star->next_point;
                if(p_cur_star == *pp_head_star) break; 
            }        
        }
        p_cur_client = p_cur_client->next_client;
    }
}

int is_point_same(int x1, int y1, int x2, int y2){
    if((x1 == x2) && (y1 == y2)) return 1;
    return 0;
}

void eat_star(CLIENT* p_client_data){
    p_client_data->score += 1;
    p_client_data->remain_tail += 1;
}

void collision_check_c2c(CLIENT_NODE* p_head_client, int n_clients, POINT_NODE** pp_head_star, int* p_nstars){
    CLIENT_NODE* p_cur_client = p_head_client;
    int i = 0;
    for(i; i < n_clients; ++i){
        if(p_cur_client->client_data.alive == 1){
            CLIENT_NODE* p_counter_client = p_cur_client->next_client;
            while(1){
                if(p_counter_client->client_data.alive == 1){
                    if(is_point_in_AABB((*(p_cur_client->client_data.pp_head_point))->point.x, (*(p_cur_client->client_data.pp_head_point))->point.y
                                            , &(p_counter_client->client_data.collision_box)))
                    {
                        if(is_crash((*(p_cur_client->client_data.pp_head_point))->point.x, (*(p_cur_client->client_data.pp_head_point))->point.y
                                            , *(p_counter_client->client_data.pp_head_point), p_counter_client->client_data.n_points))
                        {
                            p_cur_client->client_data.collision = 1;
                        }
                    }
                } 
                p_counter_client = p_counter_client->next_client;
                if(p_counter_client == p_cur_client) break;
            }
        }  
        p_cur_client = p_cur_client->next_client;
    }

    // 서로 겹쳐서 충돌하는 경우를 위해 마지막에 alive 바꿔주기
    p_cur_client = p_head_client;
    for(i = 0; i < n_clients; ++i){
        // 충돌한 클라이언트 처리
        if(p_cur_client->client_data.collision == 1){
            process_crashed_client(&(p_cur_client->client_data), pp_head_star, p_nstars);
        }
        p_cur_client = p_cur_client->next_client;
    }
}

void process_crashed_client(CLIENT* p_client_data, POINT_NODE** pp_head_star, int* p_nstars){
    POINT_NODE* p_head_point = *(p_client_data->pp_head_point);
    // 별 남기기
    create_points2star(p_head_point, pp_head_star, p_nstars);
    // data 정리
    while(p_head_point != p_head_point->next_point){
        delete_point(p_head_point->next_point, p_client_data->pp_head_point);
    }
    p_client_data->alive = 0;
    p_client_data->collision = 0;
    p_client_data->death_time = clock();
    p_client_data->score = -1;
    p_client_data->n_points = 1;
}

int is_point_in_AABB(int x, int y,  AABB* p_aabb){
    if((p_aabb->left <= x && x <= p_aabb->right) && (p_aabb->bottom <= y && y <= p_aabb->top))return 1;
    return 0;
}

int is_crash(int x, int y, POINT_NODE* p_head_point, int n_points){
    POINT_NODE* p_cur_point = p_head_point;
    int i = 0;
    for(i; i < n_points - 1; ++i){
        POINT_NODE* p1 = p_cur_point;
        POINT_NODE* p2 = p_cur_point->next_point;
        int left = (p1->point.x <= p2->point.x) ? p1->point.x : p2->point.x;
        int right = (p1->point.x >= p2->point.x) ? p1->point.x : p2->point.x;
        int bottom = (p1->point.y <= p2->point.y) ? p1->point.y : p2->point.y;
        int top = (p1->point.y >= p2->point.y) ? p1->point.y : p2->point.y;
        if((left <= x && x <= right) && (bottom <= y && y <= top)){ return 1; }
        p_cur_point = p_cur_point->next_point;
    }
    return 0;
}

void move_clients(CLIENT_NODE* p_head_client, int n_clients){
    CLIENT_NODE* p_cur_client = p_head_client;
    int i = 0;
    for(i; i < n_clients; ++i){
        if(p_cur_client->client_data.alive == 1) move(&(p_cur_client->client_data));

        p_cur_client = p_cur_client->next_client;
    }
}

void move(CLIENT* p_client_data){
    // boost 가능 여부 확인
    if((p_client_data->input.boost == 'o') && (p_client_data->score <= 0)) p_client_data->input.boost = 'x';

    int head_speed = 5;
    int tail_speed = 5;

    if(p_client_data->remain_tail > 0){
		p_client_data->use = 1;
        tail_speed = 0;
    }
    
    if(p_client_data->input.boost == 'o'){
        head_speed = 10;
        tail_speed = 10;
    }

    move_head(head_speed, p_client_data);
    move_tail(tail_speed, p_client_data);
    update_AABB(p_client_data);
}

void move_head(int speed, CLIENT* p_client_data){
    int x_dir = 0;
    int y_dir = 0;
    char dir = p_client_data->input.dir;
    switch(dir){
        case 'u':{ y_dir=1;break;}
        case 'd': {y_dir=-1;break;}
        case 'r': {x_dir=1;break;}
        case 'l': {x_dir=-1;break;}
    }
    if((p_client_data->dir) == dir){
        (*(p_client_data->pp_head_point))->point.x += x_dir*speed;
        (*(p_client_data->pp_head_point))->point.y += y_dir*speed;
    }
    else{
        POINT_NODE* p_new_point = (POINT_NODE*)malloc(sizeof(POINT_NODE));
        p_new_point->point.x =  (*(p_client_data->pp_head_point))->point.x + (x_dir*speed);
        p_new_point->point.y =  (*(p_client_data->pp_head_point))->point.y + (y_dir*speed);
        
        add_head_point(p_client_data->pp_head_point, p_new_point);
        p_client_data->n_points += 1;
        p_client_data->dir = dir;
    }
}

void move_tail(int speed, CLIENT* p_client_data){
    POINT_NODE* p_tail_point = (*(p_client_data->pp_head_point))->prev_point;
    POINT_NODE* p_pre_tail_point = p_tail_point->prev_point;

    int x_dir = (p_pre_tail_point->point.x - p_tail_point->point.x);
    int y_dir = (p_pre_tail_point->point.y - p_tail_point->point.y);
    if(x_dir != 0) x_dir = x_dir/abs(x_dir);
    if(y_dir != 0) y_dir = y_dir/abs(y_dir);

    p_tail_point->point.x += x_dir*speed;
    p_tail_point->point.y += y_dir*speed;

    if((p_tail_point->point.x == p_pre_tail_point->point.x) && (p_tail_point->point.y == p_pre_tail_point->point.y)){
        delete_point(p_tail_point, p_client_data->pp_head_point);
        p_client_data->n_points -= 1;
    }
}



void create_star(POINT_NODE** pp_head_star, int* p_nstars, clock_t* p_last_star_time){
    clock_t cur_time = clock();
    double diff = (double)(cur_time - *p_last_star_time)/CLOCKS_PER_SEC;
    if(diff >= 0.5){
        int x = (rand()%((MAP_WIDTH/10)-5)) + 10;
        int y = (rand()%((MAP_HEIGHT/10)-5)) + 10;
        x = x * 10;
        y = y * 10;

        POINT_NODE* p_new_star = (POINT_NODE*)malloc(sizeof(POINT_NODE));
        p_new_star->point.x = x;
        p_new_star->point.y = y;
        add_head_point(pp_head_star, p_new_star);
        *p_nstars += 1;
        *p_last_star_time = clock();
    }
}

void process_dead_clients(CLIENT_NODE* p_head_client, int n_clients){
    clock_t cur_time = clock();
    CLIENT_NODE* p_cur_client = p_head_client;
    int i =0;
    for(i; i < n_clients; ++i){
        if(p_cur_client->client_data.alive == 0){
            // 죽은지 3초 후에 다시 시작 + 초기화
            double diff = (double)(cur_time - p_cur_client->client_data.death_time)/CLOCKS_PER_SEC;
            if(diff > 3.0){
                reset_client_data(&(p_cur_client->client_data), p_head_client, n_clients);
            }
        }
        p_cur_client = p_cur_client->next_client;
    }
}

void reset_client_data(CLIENT* p_client_data, CLIENT_NODE* p_head_client, int n_clients){
    // 생성할 위치 찾기 15번 시도 -> 안되면 error, exit
    int x;
    int y;
    int i = 0;
    for(i ; i < 15; ++i){
        x = (rand()%((MAP_WIDTH/10)-26)) + 10;
        y = (rand()%((MAP_HEIGHT/10)-26)) + 10;
        x = x * 10;
        y = y * 10;
        AABB aabb;
        aabb.left = x; aabb.right = x + 60; aabb.bottom = y; aabb.top = y + 60;
            if(is_position_OK(p_head_client, &aabb) == 1) break;
    }    
    if(i == 15){
        printf("space error\n"); exit(1);
    }
    x += 30; y += 30;

    // 기존 head point 한개 존재 -> tail로
    p_client_data->color = (rand()%64) + 1;
    POINT_NODE* p_new_point = (POINT_NODE*)malloc(sizeof(POINT_NODE));  // head가 되는 point
    switch(rand()%4){
        case 0: // 가로 'l'
        {
            (*(p_client_data->pp_head_point))->point.x = x + 20;
            (*(p_client_data->pp_head_point))->point.y = y;
            p_new_point->point.x = x - 20;
            p_new_point->point.y = y;
            add_head_point(p_client_data->pp_head_point, p_new_point);
            p_client_data->dir = 'l';
            p_client_data->input.dir = 'l';
            break;
        }
        case 1: // 가로 'r'
        {
            (*(p_client_data->pp_head_point))->point.x = x - 20;
            (*(p_client_data->pp_head_point))->point.y = y;
            p_new_point->point.x = x + 20;
            p_new_point->point.y = y;
            add_head_point(p_client_data->pp_head_point, p_new_point);
            p_client_data->dir = 'r';
            p_client_data->input.dir = 'r';
            break;
        }
        case 2: // 세로 'd'
        {
            (*(p_client_data->pp_head_point))->point.x = x;
            (*(p_client_data->pp_head_point))->point.y = y + 20;
            p_new_point->point.x = x;
            p_new_point->point.y = y - 20;
            add_head_point(p_client_data->pp_head_point, p_new_point);
            p_client_data->dir = 'd';
            p_client_data->input.dir = 'd';
            break;
        }
        case 3: // 세로 'u'
        {
            
            (*(p_client_data->pp_head_point))->point.x = x;
            (*(p_client_data->pp_head_point))->point.y = y - 20;
            p_new_point->point.x = x;
            p_new_point->point.y = y + 20;
            add_head_point(p_client_data->pp_head_point, p_new_point);
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
}


void send_data_to_clients(VECTOR* p_send_data, CLIENT_NODE* p_head_client, int n_clients
                            , POINT_NODE* p_head_star, int n_stars)
{
    unsigned int data_size = 0;
    data_size = get_send_data_size(p_head_client, n_clients, n_stars);
    check_vector(p_send_data, data_size);
    fill_send_data(p_send_data, p_head_client, n_clients, p_head_star, n_stars);

    CLIENT_NODE* p_cur_client = p_head_client;
    int i = 0;
    for(i; i < n_clients; ++i){
        send_data_to_client(data_size, p_send_data, p_cur_client->client_data.write_fd);

        p_cur_client = p_cur_client->next_client;
    }
}

void send_data_to_client(unsigned int data_size, VECTOR* p_send_data, int write_fd){
    if(write(write_fd, &data_size, sizeof(unsigned int)) == -1){
        perror("message write failed"); //exit(1);
    }
    if(write(write_fd, p_send_data->p_data, data_size) == -1){
        perror("message write failed"); //exit(1);
    }
}


unsigned int get_send_data_size(CLIENT_NODE* p_head_client, int n_clients, int n_stars){
    // nUser + (score + color + lenID + id + nPoints + points) + (...) + (...) 
    //  + nStar + nStar*(x + y)
    int i = 0;
    unsigned int data_size = 0;
    
    data_size += sizeof(int);
    CLIENT_NODE* p_cur_client = p_head_client;
    for(i; i < n_clients; ++i){
        data_size += sizeof(int);
        data_size += sizeof(unsigned char);
        data_size += sizeof(unsigned int);
        data_size += sizeof(char) * (p_cur_client->client_data.len_id);
        data_size += sizeof(unsigned int);
        data_size += sizeof(int) * 2 * (p_cur_client->client_data.n_points);
        
        p_cur_client = p_cur_client->next_client;
    }
    
    data_size += sizeof(int);
    data_size += sizeof(int) * 2 * n_stars;

    return data_size;
}

void check_vector(VECTOR* p_vector, unsigned int need){
    if(p_vector->capacity < need){
        if(p_vector->p_data) free(p_vector->p_data);
        p_vector->p_data = (char*)malloc(need * 2);
        p_vector->capacity = need * 2;
    }
    p_vector->data_size = need;
}

void fill_send_data(VECTOR* p_send_data, CLIENT_NODE* p_head_client, int n_clients, POINT_NODE* p_head_star , int n_stars){
    char* p_data = p_send_data->p_data;

    // nUser + (score + color + lenID + id + nPoints + points) + (...) + (...) 
    //  + nStar + nStar*(x + y)
    memcpy(p_data, &n_clients, sizeof(int));
    p_data += sizeof(int);
    
    int i = 0;
    int j = 0;
    CLIENT_NODE* p_cur_client = p_head_client;
    for( i; i < n_clients; ++i){
        memcpy(p_data ,&(p_cur_client->client_data.score), sizeof(int));
        p_data += sizeof(int);
        memcpy(p_data ,&(p_cur_client->client_data.color), sizeof(unsigned char));
        p_data += sizeof(unsigned char);
        memcpy(p_data ,&(p_cur_client->client_data.len_id), sizeof(unsigned int));
        p_data += sizeof(unsigned int);
        memcpy(p_data ,p_cur_client->client_data.id, p_cur_client->client_data.len_id);  //null은 copy하지 않는다
        p_data += p_cur_client->client_data.len_id;
        memcpy(p_data ,&(p_cur_client->client_data.n_points), sizeof(unsigned int));
        p_data += sizeof(unsigned int);

        POINT_NODE* p_cur_point = *(p_cur_client->client_data.pp_head_point);
        for(j = 0; j < p_cur_client->client_data.n_points; ++j){
            memcpy(p_data, &(p_cur_point->point.x), sizeof(int));
            p_data += sizeof(int);
            memcpy(p_data, &(p_cur_point->point.y), sizeof(int));
            p_data += sizeof(int);
            
            p_cur_point = p_cur_point->next_point;
        }

        p_cur_client = p_cur_client->next_client;
    }

    memcpy(p_data, &n_stars, sizeof(int));
    p_data += sizeof(int);
    POINT_NODE* p_cur_star = p_head_star;

    for(j = 0; j < n_stars; ++j){
        memcpy(p_data, &(p_cur_star->point.x), sizeof(int));
        p_data += sizeof(int);
        memcpy(p_data, &(p_cur_star->point.y), sizeof(int));
        p_data += sizeof(int);
        
        p_cur_star = p_cur_star->next_point;
    }
}
