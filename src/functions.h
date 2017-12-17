#ifndef FUNCTIONS_H
#define FUNCTIONS_H
#include "data_struct.h"

int is_AABB_collise( AABB*,  AABB*);

void add_head_point(POINT_NODE**, POINT_NODE*);

void update_AABB(CLIENT*);

void create_points2star(POINT_NODE*, POINT_NODE**, int*);
void delete_point(POINT_NODE*, POINT_NODE**);

#endif