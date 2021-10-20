#pragma once
#include <pthread.h>

#include "shared_memory.h"

// car object used while car is waiting in entrance queue.
typedef struct car {

    int entrance;

    char license_plate[6];

} car_t;

// car object used after car has been assigned a thread.
typedef struct car_request {

    char license_plate[6];
    
    int level;

    struct car_request *next;

} car_request_t;

// list of cars waiting to be assigned a thread.
typedef struct car_request_queue {

    car_request_t *head;

    car_request_t *tail;

    pthread_mutex_t mutex;
    
    pthread_cond_t cond;

} car_request_queue_t;

typedef struct car_request_args {
    
    car_request_queue_t *car_request_queue;
    
    shared_data_t *data;

    pthread_mutex_t *rng_mutex;

    int exit_count;

} car_request_args_t;

void *generate_cars(void *car_args);

void add_car_request(car_request_queue_t *car_request_queue, car_t car, int level);

void *handle_car_requests(void *car_request_args);