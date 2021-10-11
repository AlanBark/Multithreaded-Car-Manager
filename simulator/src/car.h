#pragma once
#include <pthread.h>

typedef struct car {
    
    int thread_id;

    int entrance;

    char license_plate[6];

} car_t;

void *generate_cars(void *car_args);