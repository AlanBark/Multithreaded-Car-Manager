#pragma once
#include <pthread.h>

typedef struct gate {

    pthread_mutex_t mutex;

    pthread_cond_t cond;

    char status;

} gate_t;

void initialize_gate(gate_t*);

void update_gate(gate_t*, char);

char get_gate(gate_t* gate);