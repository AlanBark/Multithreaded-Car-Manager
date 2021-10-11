#include <stdlib.h>
#include <pthread.h>
#include "queue.h"

void queue_initialize(queue_t *queue) {
    pthread_mutex_init(&(queue->mutex), NULL);
    pthread_cond_init(&(queue->cond), NULL);
    queue->head = NULL;
    queue->tail = NULL;
}

void queue_add(queue_t* queue, car_t car) {
    queue_node_t* node = malloc(sizeof(queue_node_t));
    node->car = car;
    if (queue->head == NULL) {
        pthread_mutex_lock(&queue->mutex);
        queue->head = node;
        queue->tail = node;
        pthread_cond_signal(&queue->cond);
        pthread_mutex_unlock(&queue->mutex);
    } else {
        queue->tail->next = node;
        queue->tail = node;
    }
}