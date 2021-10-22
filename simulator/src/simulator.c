#include <stdbool.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "car.h"
#include "queue.h"
#include "shared_memory.h"
#include "util.h"

#define SHARED_NAME "PARKING"

/* Constants controlling parking lot size*/

#define LEVEL_COUNT 5
#define ENTRANCE_COUNT 5
#define EXIT_COUNT 5
#define CARS_PER_LEVEL 20

void *run_gates(void *gate_arg) {
    gate_t *gate;
    gate = (gate_t *) gate_arg;
    
    // always keep gates alive
    while (true) {

        pthread_mutex_lock(&gate->mutex);
        // sleep until gates are either R or L
        while (gate->status != 'R' && gate->status != 'L') {
            pthread_cond_wait(&gate->cond, &gate->mutex);
        }
        // sleep then change gate state accordingly
        pthread_mutex_unlock(&gate->mutex);
        ms_sleep(10);
        if (gate->status == 'R') {
            update_gate(gate, 'O');
        } else if (gate->status == 'L') {
            update_gate(gate, 'C');
        }
    }
    pthread_exit(NULL);
}

void *run_entrances(void *entrance_args) {
    entrance_args_t *args;
    args = (entrance_args_t*) entrance_args;

    pthread_t gate_tid;
    pthread_create(&gate_tid, NULL, run_gates, (void*) &args->entrance->gate);
    
    // entrance stays alive until program ends.
    pthread_mutex_lock(&args->queue->mutex);

    while (true) {

        // Wait on new cars to avoid busy waiting.
        while (args->queue->head == NULL) {
            pthread_cond_wait(&args->queue->cond, &args->queue->mutex);
        }

        // A car has arrived! Time to sleep
        ms_sleep(2);

        // update relevant lpr and wait for the sign to update accordingly
        update_plate(&args->entrance->lpr, args->queue->head->car.license_plate);

        pthread_mutex_lock(&args->entrance->sign.mutex);
        pthread_cond_wait(&args->entrance->sign.cond, &args->entrance->sign.mutex);
        
        /* Check if car was assigned a space */
        if (isdigit(args->entrance->sign.display)) {
            int level = args->entrance->sign.display - '0';
            pthread_mutex_unlock(&args->entrance->sign.mutex);

            /* Wait for gate to open before assigning a thread to the car */
            pthread_mutex_lock(&args->entrance->gate.mutex);
            while (args->entrance->gate.status != 'O') {
                pthread_cond_wait(&args->entrance->gate.cond, &args->entrance->gate.mutex);
            }
            pthread_mutex_unlock(&args->entrance->gate.mutex);
            
            /* Car has made it through the gate, request for a thread to handle the car */
            add_car_request(args->car_request_queue, args->queue->head->car, level);
        } else {
            pthread_mutex_unlock(&args->entrance->sign.mutex);
        }

        // remove cars from queue and free queue memory.
        queue_node_t *next = args->queue->head->next;
        free(args->queue->head);
        args->queue->head = next;
    }
    pthread_exit(NULL);
}

int main(int argc, char **argv) {

    /* Shared memory setup */
    shared_memory_t shm;

    if (!create_shared_object(&shm, SHARED_NAME, true)) {
        fprintf(stderr, "ERROR Failed to create shared object\n");
        return 1;
    }

    if (!initialize_shared_object(&shm, ENTRANCE_COUNT, EXIT_COUNT, LEVEL_COUNT)) {
        fprintf(stderr, "ERROR Failed to initialize shared object\n");
        return 1;
    }

    /* Random number setup */
    srand(time(0));
    pthread_mutex_t rng_mutex;
    pthread_mutex_init(&rng_mutex, NULL);

    /* Thread ID setup */
    pthread_t entrance_threads[ENTRANCE_COUNT];
    pthread_t car_factory_tid;
    
    /* create enough blocked threads to handle max capacity carpark, 
    plus some more for cars in transit between exits/entrances */
    int max_cars = LEVEL_COUNT * (CARS_PER_LEVEL + 10);
    pthread_t car_requests[max_cars];

    car_request_queue_t car_request_queue;
    pthread_cond_init(&car_request_queue.cond, NULL);
    pthread_mutex_init(&car_request_queue.mutex, NULL);
    car_request_queue.head = NULL;
    car_request_queue.tail  = NULL;

    car_request_args_t car_request_args;
    car_request_args.car_request_queue = &car_request_queue;
    car_request_args.data = shm.data;
    car_request_args.rng_mutex = &rng_mutex;
    car_request_args.exit_count = EXIT_COUNT;

    for (int i = 0; i < max_cars; i++) {
        pthread_create(&car_requests[i], NULL, handle_car_requests, (void *)&car_request_args);
    }

    /* Entrance Queue Setup */
    queue_t **entrance_queues = malloc(sizeof(queue_t*)*ENTRANCE_COUNT);
    entrance_args_t entrance_args[ENTRANCE_COUNT];

    for (int i = 0; i < ENTRANCE_COUNT; i++) {
        entrance_queues[i] = malloc(sizeof(queue_t));
        queue_initialize(entrance_queues[i]);

        entrance_args[i].queue = entrance_queues[i];
        entrance_args[i].entrance = &shm.data->entrance_collection[i];
        entrance_args[i].car_request_queue = &car_request_queue;
        pthread_create(&entrance_threads[i], NULL, run_entrances, (void *)&entrance_args[i]);
    }

    /* Exit boom gates setup */
    pthread_t exit_gates[EXIT_COUNT];
    for (int i = 0; i < EXIT_COUNT; i++) {
        pthread_create(&exit_gates[i], NULL, run_gates, (void *)&shm.data->exit_collection[i].gate);
    }

    /* Car factory setup */
    car_args_t car_args;
    car_args.queues = entrance_queues;
    car_args.entrance_count = ENTRANCE_COUNT;
    car_args.rng_mutex = &rng_mutex;

    pthread_create(&car_factory_tid, NULL, generate_cars, (void *)&car_args);

    pthread_join(entrance_threads[0], NULL);
    return 0;
}