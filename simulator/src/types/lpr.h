#pragma once
#include <pthread.h>

typedef struct lpr {

    pthread_mutex_t mutex;

    pthread_cond_t cond;

    char license_plate[6];

} lpr_t;

void initialize_lpr(lpr_t*);

void update_plate(lpr_t*, char[6]);
