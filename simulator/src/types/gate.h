#pragma once
#include <pthread.h>

typedef struct gate {

    pthread_mutex_t mutex;

    pthread_cond_t cond;

    char status;

} gate_t;

void initialize_gate(gate_t*);

void update_status(gate_t*, char);
