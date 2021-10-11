#pragma once
#include <pthread.h>
#include "car.h"
#include "entrance.h"

typedef struct queue_node {

    car_t car;

    struct queue_node* next;

} queue_node_t;

typedef struct queue {

    queue_node_t *head;

    queue_node_t *tail;

    pthread_mutex_t mutex;

    pthread_cond_t cond;

} queue_t;

typedef struct car_args {

    int entrance_count;

    pthread_mutex_t *rng_mutex;

    queue_t **queues;

} car_args_t;

typedef struct entrance_args {

    queue_t *queue;

    entrance_t entrance;

} entrance_args_t;

void queue_initialize(queue_t *queue);

void queue_add(queue_t* queue, car_t car);