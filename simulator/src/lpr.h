#pragma once
#include <pthread.h>

typedef struct lpr {

    pthread_mutex_t mutex;

    pthread_cond_t condition;

    char licensePlate[6];

} lpr_t;

void updatePlate(lpr_t*, char[6]);