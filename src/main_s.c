#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include "update.h"


///////////////////////////////////////
pthread_t thread;

pthread_mutex_t client_data;
CLIENT_NODE** pp_head_client;
int n_clients;

POINT_NODE** pp_head_star;
int n_stars;

sem_t client;
///////////////////////////////////////


void listen(void*);
void update(void*);

void sig_handler(int signo){}


int main(){
    
    srand(time(NULL));
    signal(SIGPIPE, sig_handler);

    // 자원 초기화
    pp_head_client = (CLIENT_NODE**)malloc(sizeof(CLIENT_NODE*));
    *pp_head_client = NULL;
    n_clients = 0;

    pp_head_star = (POINT_NODE**)malloc(sizeof(POINT_NODE*));
    *pp_head_star = NULL;
    n_stars = 0;
    pthread_mutex_init(&client_data, NULL);
    sem_init(&client, 0, 0);

    pthread_create(&thread, NULL, (void*)listen, NULL);
    update(NULL);

    void* msg;
    pthread_join(thread, &msg);

    // 자원들 해제
    pthread_mutex_destroy(&client_data);
    sem_destroy(&client);
    while(n_clients){
        process_out_client(*pp_head_client, pp_head_client, &n_clients, pp_head_star, &n_stars, &client);
    }
    while(n_stars){
        delete_point(*pp_head_star, pp_head_star);
        n_stars -= 1;
    }
}


void update(void* p){
    clock_t last_frame_time = clock();
    clock_t last_star_time = clock();
    VECTOR send_data;
    send_data.capacity = 0;
    send_data.p_data = NULL;

    while(1){
        sem_wait(&client);
        sem_post(&client);
        // recv inputs
        clock_t cur_time = clock();
        double diff = (double)(cur_time - last_frame_time)/CLOCKS_PER_SEC;
        if(diff < 0.5){
            pthread_mutex_lock(&client_data);
            //printf("read clients data \n");
            read_clients_input(pp_head_client, &n_clients, pp_head_star, &n_stars, &client);
            pthread_mutex_unlock(&client_data);
            continue;
        }

        // update
        pthread_mutex_lock(&client_data);

        update_world(*pp_head_client, n_clients, pp_head_star, &n_stars, &last_star_time);
        send_data_to_clients(&send_data, *pp_head_client, n_clients, *pp_head_star, n_stars);

        pthread_mutex_unlock(&client_data);
    }
}


void listen(void* p){
    char* fifo = "listen_fifo";
    int fd;

    // make fifo
    if(mkfifo(fifo, 0666) == -1){
        if(errno != EEXIST){
            printf("fifo erro"); exit(1);
        }
    }

    // open fifo
    if((fd = open(fifo, O_RDONLY)) < 0){
        printf("fifo erro"); exit(1);
    }

    // 메시지 받기
    while(1){
        pid_t pid;
        unsigned int len;
        char id_buf[11];
        int ret;
        
        if((ret = read(fd, &pid, sizeof(pid_t))) == 0){
            close(fd);
            if((fd = open(fifo, O_RDONLY)) < 0){ printf("fifo erro"); exit(1); }
            continue;
        }
        else if(ret < 0){ printf("fifo read pid error"); exit(1); }

        if(read(fd, &len, sizeof(unsigned int)) < 0){
            printf("fifo read id_len error"); exit(1);
        }

        if(read(fd, id_buf, len) < 0){
            printf("fifo read id error"); exit(1);
        }
        id_buf[len] = 0;

        // 검증 (이미 존재하는 id인지, 자리가 있는지) +  등록
        int id_exist = 0;
        pthread_mutex_lock(&client_data);
        if(*pp_head_client){
            id_exist = is_exist_id(*pp_head_client, id_buf, len);
        }
        if(id_exist == 0){
            id_exist = register_client(pp_head_client, &n_clients, pid, len, id_buf);
        }
        if(id_exist == 0) n_clients += 1;
        pthread_mutex_unlock(&client_data);

        // signal 보내주기
        if(id_exist == 0){  //ok
            kill(pid, SIGUSR1);
            sem_post(&client);
        }
        else{   // reject
            kill(pid, SIGUSR2);
        }
    }
}



// void write(void*);
/*
sem_t write_buffer_ready;
sem_t write_buffer_empty;
VECTOR write_buffer;    // data
VECTOR write_fds;       // int vector 처럼 사용
*/

/*
void write(void* p){
    while(1){
        sem_wait(&write_buffer_ready);
        send_data_to_clients(&write_fds, &write_buffer);
        sem_post(&write_buffer_empty);
    }
}
*/
