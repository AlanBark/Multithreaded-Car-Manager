#pragma once
#include <pthread.h>

typedef struct sign {

    pthread_mutex_t mutex;

    pthread_cond_t cond;

    char display;

} sign_t;

void initialize_sign(sign_t*);

void update_display(sign_t*, char);
