#include <stdlib.h>
#include <pthread.h>
#include "queue.h"

void queue_initialize(queue_t *queue) {
    pthread_mutex_init(&(queue->mutex), NULL);
    pthread_cond_init(&(queue->cond), NULL);
    queue->head = NULL;
    queue->tail = NULL;
}