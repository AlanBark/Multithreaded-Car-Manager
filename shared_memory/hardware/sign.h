#pragma once
#include <pthread.h>

typedef struct sign {

    pthread_mutex_t mutex;

    pthread_cond_t cond;

    char display;

} sign_t;

void initialize_sign(sign_t*);

void update_sign(sign_t*, char);

char get_sign(sign_t* sign);