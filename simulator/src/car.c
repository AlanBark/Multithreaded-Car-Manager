#include <stdbool.h>
#include <stdio.h> // @TODO remove
#include <stdlib.h>
#include "car.h"
#include "entrance.h"
#include "util.h"
#include "queue.h"

/* Generates a new car every 1-100ms with a random plate and entrance */
void *generate_cars(void *car_args) {
    car_args_t *args;
    args = (car_args_t *) car_args;

    pthread_mutex_t *rng_mutex = args->rng_mutex;
    int entrance_count = args->entrance_count;
    
    while (true) {
        int delay = get_random_number(rng_mutex, 1, 100);
        ms_sleep(delay);

        car_t car;

        // @TODO make this less shit
        car.license_plate[0] = get_random_number(rng_mutex, 48, 57);
        car.license_plate[1] = get_random_number(rng_mutex, 48, 57);
        car.license_plate[2] = get_random_number(rng_mutex, 48, 57);
        car.license_plate[3] = get_random_number(rng_mutex, 65, 90);
        car.license_plate[4] = get_random_number(rng_mutex, 65, 90);
        car.license_plate[5] = get_random_number(rng_mutex, 65, 90);
        int entrance = get_random_number(rng_mutex, 1, entrance_count);
        printf("Added car %s to %d, took %dms\n", car.license_plate, entrance, delay);

        queue_node_t* node = malloc(sizeof(queue_node_t));
        node->car = car;

        if (args->queues[entrance - 1]->head == NULL) {
            pthread_mutex_lock(&args->queues[entrance - 1]->mutex);
            args->queues[entrance - 1]->head = node;
            args->queues[entrance - 1]->tail = node;
            pthread_cond_signal(&args->queues[entrance - 1]->cond);
            pthread_mutex_unlock(&args->queues[entrance - 1]->mutex);
        } else {
            args->queues[entrance - 1]->tail->next = node;
            args->queues[entrance - 1]->tail = node;
        }
        // queue_add(args->queues[entrance - 1], car);
    }
}