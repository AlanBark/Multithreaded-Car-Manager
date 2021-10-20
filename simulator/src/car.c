#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
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

        queue_node_t* node = malloc(sizeof(queue_node_t));
        memset(node, 0, sizeof(queue_node_t));
        node->car = car;
        node->next = NULL;

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
    }
}

/* Convert car data into a car request, and add this request to the request queue. */
void add_car_request(car_request_queue_t *car_request_queue, car_t car, int level) {
    /* allocate memory for a car request, fill license plate data */
    car_request_t *car_request;
    car_request = malloc(sizeof(car_request_t));
    memcpy(car_request->license_plate, car.license_plate, 6);
    car_request->next = NULL;
    car_request->level = level;

    pthread_mutex_lock(&car_request_queue->mutex);
    if (car_request_queue->head == NULL) {
        car_request_queue->head = car_request;
        car_request_queue->tail = car_request;
    } else {
        car_request_queue->tail->next = car_request;
    }
    pthread_cond_signal(&car_request_queue->cond);
    pthread_mutex_unlock(&car_request_queue->mutex);
}

/* Checks the car request queue, when there is a car_t object in the queue, and
   This thread will handle the actions of that car until the car leaves the lot.
   The thread will then return to watching the car queue.*/
void *handle_car_requests(void *car_request_args) {
    car_request_args_t *args;
    args = (car_request_args_t *) car_request_args;

    car_request_queue_t *car_request_queue = args->car_request_queue;
    shared_data_t *data = args->data;
    pthread_mutex_t *rng_mutex = args->rng_mutex;
    int exit_count = args->exit_count;

    while (true) {
        pthread_mutex_lock(&car_request_queue->mutex);

        /* Wait for a car to appear in the queue */
        while (car_request_queue->head == NULL) {
            pthread_cond_wait(&car_request_queue->cond, &car_request_queue->mutex);
        }
        /* Got a car, update queue before releasing lock */
        car_request_t *current_car = car_request_queue->head;
        if (current_car->next == NULL) {
            car_request_queue->head = NULL;
            car_request_queue->tail = NULL; // Not really necessary but might as well.
        } else {
            car_request_queue->head = current_car->next;
        }
        pthread_mutex_unlock(&car_request_queue->mutex);

        /* Sleep for 10ms before updating the car's position - Timings. */
        ms_sleep(10);

        /* Add the car's plates to the current level's lpr.*/
        update_plate(&data->level_collection[current_car->level - 1].lpr, current_car->license_plate);

        /* Sleep for a period of 100 - 10000 ms*/
        ms_sleep(get_random_number(rng_mutex, 100, 10000));

        /* Leave the current level, triggering the lpr again */
        update_plate(&data->level_collection[current_car->level - 1].lpr, current_car->license_plate);

        /* Take 10ms to reach exit */
        ms_sleep(10);

        /* Go to a random exit and trigger the lpr */
        int exit = get_random_number(rng_mutex, 1, exit_count);
        update_plate(&data->exit_collection[exit - 1].lpr, current_car->license_plate);

        /* Wait for exit boom gates to open then leave */
        // pthread_mutex_lock(&data->exit_collection[exit - 1].gate.mutex);
        // while (&data->exit_collection[exit - 1].gate.status != 'O') {
        //     pthread_cond_wait(&data->exit_collection[exit - 1].gate.cond, &data->exit_collection[exit - 1].gate.mutex);
        // }
        // pthread_mutex_unlock(&data->exit_collection[exit - 1].gate.mutex);

        /* Clean up before waiting for another car request */
        free(current_car);
    }

    pthread_exit(NULL);
}