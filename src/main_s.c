#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>
#include <string.h>
#include "functions.h"
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>


pthread_t thread[2];
/////////////공유 데이터//////////////////// 

pthread_mutex_t data;
CLIENT* head_client = NULL;
int n_clients = 0;
POINT* head_star = NULL;
int n_stars  = 0;

////////////////////////////////////////////

void update(void*);
void listen(void*);


int main(){
    int i;
    void* msg;

    srand(time(NULL));
    // 자원 초기화
    pthread_mutex_init(&data, NULL);
    pthread_create(&thread[0], NULL, (void*)listen, NULL);
    pthread_create(&thread[1], NULL, (void*)update, NULL);


    for(i = 0; i < 2; ++i){
        pthread_join(thread[i], &msg);
    }
    pthread_mutex_destroy(&data);

    // 자원들 해제
    
}

void update(void* p){
    time_t last_frame_time;
    time_t last_star_time;
    time(&last_frame_time);
    time(&last_star_time);
    VECTOR vector;
    vector.capacity = 0;
    vector.p_data = NULL;
    while(1){
        //recv inputs
        time_t cur_time;
        time(&cur_time);
        while(difftime(cur_time, last_frame_time) < 0.0333){
            pthread_mutex_lock(&data);
            if(head_client)read_clients_input(&head_client, &head_star, &n_clients, &n_stars);
            pthread_mutex_unlock(&data);

            time(&cur_time);
        }

        time(&last_frame_time);
        
        //update world
        pthread_mutex_lock(&data);

        if(head_client){
            create_star(&last_star_time, &head_star, &n_stars);
            update_world(&head_client, &head_star, &n_clients, &n_stars);
            send_result(&vector, head_client, head_star, n_clients, n_stars);
        }
        pthread_mutex_unlock(&data);
    }
}

void sig_handle(int signo) {
}

void listen(void* p){
    char* fifo = "listen_fifo";
    int fd;
    // make fifo
    if(mkfifo(fifo, 0666) == -1){
        if(errno != EEXIST){
            printf("fifo error"); exit(1);
        }
    }

    // open fifo
    if((fd = open(fifo, O_RDONLY)) < 0){
        printf("fifo error"); exit(1);
    }

	signal(SIGUSR1, sig_handle);

    // 메시지 받는다
    while(1){
        pid_t pid;
        unsigned int len;
        char id_buf[11];
		int ret;
        if((ret = read(fd, &pid, sizeof(pid_t))) == 0) {
			close(fd);
			if((fd = open(fifo, O_RDONLY)) < 0){
				printf("fifo error"); exit(1);
			}
			continue;
		}
		else if(ret < 0) {
            printf("fifo read pid error"); exit(1);
		}

		printf("read pid\n");
        if(read(fd, &len, sizeof(unsigned int)) < 0){
            printf("fifo read id_len error"); exit(1);
        }
        if(read(fd, id_buf, len) < 0){
            printf("fifo read id error"); exit(1);
        }
        id_buf[len] = 0;

		sleep(1);

        // id 중복 확인 + 없으면 새로운 클라이언트 등록
        int id_exist = 0;
        pthread_mutex_lock(&data);
        if(head_client){
            id_exist = is_exist_id(head_client, id_buf, len);
        }
        if(id_exist == 0){
            // client 등록
            id_exist = register_client(&head_client, &n_clients, pid, len, id_buf);
        }
        pthread_mutex_unlock(&data);
        
        // signal 보내주기
        if(id_exist == 0){  // ok
            kill(pid, SIGUSR1);
        }
        else{   // reject
            kill(pid, SIGUSR2);
        }
    }

}
