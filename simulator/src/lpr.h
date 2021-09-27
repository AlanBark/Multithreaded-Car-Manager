#pragma once
#include <pthread.h>

typedef struct lpr {

    pthread_mutex_t mutex;

    pthread_cond_t cond;

    char licensePlate[6];

} lpr_t;

void updatePlate(lpr_t*, char[6]);

void initialize_lpr(lpr_t*);