#pragma once
#include <pthread.h>

typedef struct car {
    
    int thread_id;

    int entrance;

    char license_plate[6];

} car_t;


typedef struct car_args {

    int entrance_count;

    pthread_mutex_t *rng_mutex;

    queue_t **queue;

} car_args_t;

void *generate_cars(void *car_args);