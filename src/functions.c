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

int is_AABB_collise( AABB* aabb1,  AABB* aabb2){
    if(((aabb1->left < aabb2->right) && (aabb2->left < aabb1->right))
        && ((aabb1->bottom < aabb2->top) && (aabb2->bottom < aabb1->top))){
            return 1;
        }
    return 0;
}

void add_head_point(POINT_NODE** pp_head_point, POINT_NODE* p_new_point){
    if(*pp_head_point == NULL){
        p_new_point->next_point = p_new_point;
        p_new_point->prev_point = p_new_point;
    }
    else{
        p_new_point->next_point = *pp_head_point;
        p_new_point->prev_point = (*pp_head_point)->prev_point;
        (*pp_head_point)->prev_point->next_point = p_new_point;
        (*pp_head_point)->prev_point = p_new_point;
    }
    (*pp_head_point) = p_new_point;
}

void update_AABB(CLIENT* p_client_data){
    int left = MAP_WIDTH; int right = 0; int bottom = MAP_HEIGHT; int top = 0; 
    POINT_NODE* p_head_point = *(p_client_data->pp_head_point);
    POINT_NODE* p_cur_point = p_head_point;
    while(1){
        if(p_cur_point->point.x < left) left = p_cur_point->point.x;
        if(p_cur_point->point.x > right) right = p_cur_point->point.x;
        if(p_cur_point->point.y < bottom) bottom = p_cur_point->point.y;
        if(p_cur_point->point.y > top) top = p_cur_point->point.y;

        p_cur_point = p_cur_point->next_point;
        if(p_cur_point == p_head_point) break;
    }

    left -= 10; right += 10; bottom -= 10; top += 10;
    
    p_client_data->collision_box.left = left;
    p_client_data->collision_box.right = right;
    p_client_data->collision_box.bottom = bottom;
    p_client_data->collision_box.top = top;
}


void create_points2star(POINT_NODE* p_head_point, POINT_NODE** pp_head_star, int* p_nstars){
    POINT_NODE* p_cur_point = p_head_point;
    while(p_cur_point->next_point != p_head_point){
        POINT_NODE* p_next_point = p_cur_point->next_point;
        int x_dir = p_next_point->point.x - p_cur_point->point.x;
        int y_dir = p_next_point->point.y - p_cur_point->point.y;
        if(x_dir != 0) x_dir = x_dir/abs(x_dir);
        if(y_dir != 0) y_dir = y_dir/abs(y_dir);
        int x = p_cur_point->point.x;
		if(x%10 == 5) x += 5;
        int y = p_cur_point->point.y;
		if(y%10 == 5) y += 5;
        int nx = p_next_point->point.x;
		if(nx%10 == 5) nx += 5;
        int ny = p_next_point->point.y;
		if(ny%10 == 5) ny += 5;
        while(1){
            POINT_NODE* p_new_star = (POINT_NODE*)malloc(sizeof(POINT_NODE));
            p_new_star->point.x = x;
            p_new_star->point.y = y;
            add_head_point(pp_head_star, p_new_star);
            (*p_nstars) += 1;
            x += x_dir * 10;
            y += y_dir * 10;
            if(x >= nx && y >= ny) break;
        }
        p_cur_point = p_cur_point->next_point;
    }
    POINT_NODE* p_new_star = (POINT_NODE*)malloc(sizeof(POINT_NODE));
    int x = p_cur_point->point.x;
	if(x%10 == 5) x += 5;
    int y = p_cur_point->point.y;
	if(y%10 == 5) y += 5;
    p_new_star->point.x = x;
    p_new_star->point.y = y;
    add_head_point(pp_head_star, p_new_star);
    (*p_nstars) += 1;
}

void delete_point(POINT_NODE* p_del_point, POINT_NODE** pp_head_point){
    p_del_point->next_point->prev_point = p_del_point->prev_point;
    p_del_point->prev_point->next_point = p_del_point->next_point;
    if(p_del_point == *pp_head_point){
        if(p_del_point->next_point == p_del_point){
            *pp_head_point = NULL;
        }
        else *pp_head_point = p_del_point->next_point;
    }
    free(p_del_point);
}
