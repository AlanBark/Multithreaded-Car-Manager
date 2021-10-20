#include <stdbool.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "car.h"
#include "entrance.h"
#include "queue.h"
#include "exit.h"
#include "level.h"
#include "shared_memory.h"
#include "util.h"

#define SHARED_NAME "PARKING"

/* Constants controlling parking lot size*/

#define LEVEL_COUNT 5

#define ENTRANCE_COUNT 5

#define EXIT_COUNT 5

/* Amount of cars to malloc for the first queue */
#define INITIAL_QUEUE_SIZE 50

void *run_car(void *car_args) {
    pthread_exit(NULL);
}

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
        if (gate->status == 'R') {
            pthread_mutex_unlock(&gate->mutex);
            ms_sleep(10);
            update_gate(gate, 'O');
        } else if (gate->status == 'L') {
            pthread_mutex_unlock(&gate->mutex);
            ms_sleep(10);
            update_gate(gate, 'C');
        }
        // signal to other threads that gate status has changed
        pthread_cond_broadcast(&gate->cond);
        // when looping back to beginning, gate status has changed and 
        // thread will sleep as it will not be R or L
        pthread_mutex_unlock(&gate->mutex);
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
        
        // car was assigned a space, give the car a thread
        if (isdigit(args->entrance->sign.display)) {
            pthread_mutex_unlock(&args->entrance->sign.mutex);
            // wait until gate is open
            pthread_mutex_lock(&args->entrance->gate.mutex);
            while (args->entrance->gate.status != 'O') {
                pthread_cond_wait(&args->entrance->gate.cond, &args->entrance->gate.mutex);
            }
            pthread_mutex_unlock(&args->entrance->gate.mutex);
            // do stuff
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

    /* Entrance Queue Setup */
    queue_t **entrance_queues = malloc(sizeof(queue_t*)*ENTRANCE_COUNT);
    entrance_args_t entrance_args[ENTRANCE_COUNT];

    for (int i = 0; i < ENTRANCE_COUNT; i++) {
        entrance_queues[i] = malloc(sizeof(queue_t));
        queue_initialize(entrance_queues[i]);

        entrance_args[i].queue = entrance_queues[i];
        entrance_args[i].entrance = &shm.data->entrance_collection[i];

        pthread_create(&entrance_threads[i], NULL, run_entrances, (void *)&entrance_args[i]);
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