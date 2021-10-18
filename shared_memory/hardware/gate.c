#include "gate.h"
#include <string.h>
#include <pthread.h>

void initialize_gate(gate_t *gate) {

    pthread_mutexattr_t mutexAttr;
    pthread_mutexattr_init(&mutexAttr);
    pthread_mutexattr_setpshared(&mutexAttr, 1);
    pthread_mutex_init(&gate->mutex, &mutexAttr);

    pthread_condattr_t condAttr;
    pthread_condattr_init(&condAttr);
    pthread_condattr_setpshared(&condAttr, 1);
    pthread_cond_init(&gate->cond, &condAttr);
}

void update_gate(gate_t* gate, char status) {
    
    pthread_mutex_lock(&gate->mutex);
    gate->status = status;
    pthread_cond_signal(&gate->cond);
    pthread_mutex_unlock(&gate->mutex);

}

/* get gate details for status display */
// @TODO AFTER GATE IMPLEMENT
char get_gate(gate_t* gate) {
    pthread_mutex_lock(&gate->mutex);
    char status;
    if (gate->status == 0) {
        status = '-';
    } else {
        status = gate->status;
    }
    pthread_mutex_unlock(&gate->mutex);
    return status;
}