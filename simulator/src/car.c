#include <stdbool.h>

#include "car.h"
#include "entrance.h"
#include "util.h"

/* Generates a new car every 1-100ms with a random plate and entrance */
void *generate_cars(void *car_args) {
    car_args_t *args;
    args = (car_args_t *) car_args;

    pthread_mutex_t *rng_mutex = args->rng_mutex;
    int entrance_count = args->entrance_count;
    queue_t **queue = args->queue;
    
    while (true) {
        ms_sleep(get_random_number(rng_mutex, 1, 100));

        car_t car;

        // @TODO make this less shit
        car.license_plate[0] = get_random_number(rng_mutex, 48, 57);
        car.license_plate[1] = get_random_number(rng_mutex, 48, 57);
        car.license_plate[2] = get_random_number(rng_mutex, 48, 57);
        car.license_plate[3] = get_random_number(rng_mutex, 65, 90);
        car.license_plate[4] = get_random_number(rng_mutex, 65, 90);
        car.license_plate[5] = get_random_number(rng_mutex, 65, 90);
        int entrance = get_random_number(rng_mutex, 1, entrance_count);

        pthread_mutex_lock(&queue[entrance]->mutex);

        /* Double max length if queue size has been reached */
        if ((queue[entrance]->length + 1) >= queue[entrance]->max_length) {
            queue[entrance]->max_length = queue[entrance]->max_length * 2;
            queue[entrance]->cars = realloc(queue[entrance]->cars, sizeof(car_t) * queue[entrance]->max_length);
        }

        queue[entrance]->cars[queue[entrance]->length] = car;
        queue[entrance]->length++;
        
        // signal to the entrance that there is a new car in the queue.
        pthread_cond_signal(&queue[entrance]->cond);
        pthread_mutex_unlock(&queue[entrance]->mutex);
    }
}