#pragma once
#include <pthread.h>
#include "gate.h"
#include "sign.h"
#include "lpr.h"
#include "car.h"
#include "queue.h"

typedef struct entrance {

    lpr_t lpr;

    gate_t gate;

    sign_t sign;

} entrance_t;

typedef struct entrance_args {

    queue_t *queue;

    entrance_t entrance;

} entrance_args_t;


void initialize_entrance(entrance_t *entrance);

// void initialize_queues(queue_t **queue, int queue_size, int entrance_count);