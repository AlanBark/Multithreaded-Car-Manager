#pragma once
#include <pthread.h>
#include "gate.h"
#include "sign.h"
#include "lpr.h"
#include "car.h"

typedef struct entrance {

    lpr_t lpr;

    gate_t gate;

    sign_t sign;

} entrance_t;


/* Entrance queue. Contains a mutex and condition variable since this is modified by the
    Entrance thread and car factory thread.
    Length is the CURRENT amount of cars in the queue, not the amount of space that has 
    been malloced for the cars array.
 */
typedef struct queue {

    car_t *cars;

    int length;

    int max_length;

    pthread_mutex_t mutex;
    
    pthread_cond_t cond;

} queue_t;


typedef struct entrance_args {

    queue_t queue;

    entrance_t entrance;

} entrance_args_t;


void initialize_entrance(entrance_t *entrance);

void initialize_queues(queue_t **queue, int queue_size, int entrance_count);