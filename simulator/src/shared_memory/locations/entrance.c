#include <pthread.h>
#include <stdlib.h>

#include "car.h"
#include "entrance.h"


void initialize_entrance(entrance_t *entrance) {
    initialize_lpr(&entrance->lpr);
    initialize_gate(&entrance->gate);
    initialize_sign(&entrance->sign);
}

void initialize_queues(queue_t **queue, int queue_size, int entrance_count) {
    for (int i = 0; i < entrance_count; i++) {
        queue[i]->cars = malloc(sizeof(car_t) * queue_size);
        queue[i]->length = 0;
        pthread_mutex_init(&(queue[i]->mutex), NULL);
        pthread_cond_init(&(queue[i]->cond), NULL);
    }
}