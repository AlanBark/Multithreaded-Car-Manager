#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "car.h"
#include "entrance.h"
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

void *run_entrances(void *entrance_args) {
    entrance_args_t *args = (entrance_args_t*) entrance_args;
    // entrance stays alive until program ends.
    while (true) {
        pthread_mutex_lock(&args->queue.mutex);
        
        // Wait on new cars to avoid busy waiting.
        if (args->queue.length == 0) {
            pthread_cond_wait(&args->queue.cond, &args->queue.mutex);
        }

        // A car has arrived! Time to sleep
        ms_sleep(2);
        update_plate(&args->entrance.lpr, args->queue.cars[0].license_plate);

        pthread_mutex_lock(&args->entrance.sign.mutex);
        pthread_cond_wait(&args->entrance.sign.cond, &args->entrance.sign.mutex);
        if (isdigit(args->entrance.sign.display)) {
            // pthread_create(&entrance_threads[i], NULL, run_entrances, (void *)&args);
            // run_car((void*)'test');
        } 

        pthread_mutex_unlock(&args->entrance.sign.mutex);

        // @TODO this properly
        args->queue.cars++;
        args->queue.length--;
        args->queue.cars = realloc(args->queue.cars, sizeof(car_t) * args->queue.max_length);
        pthread_mutex_unlock(&args->queue.mutex);
    }
    pthread_exit(NULL);
}

int main(int argc, char **argv) {

    /* Shared memory setup */
    shared_memory_t shm;

    if (!create_shared_object(&shm, SHARED_NAME)) {
        fprintf(stderr, "ERROR Failed to create shared object");
        return 1;
    }

    if (!initialize_shared_object(&shm, ENTRANCE_COUNT, EXIT_COUNT, LEVEL_COUNT)) {
        fprintf(stderr, "ERROR Failed to initialize shared object");
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
    queue_t *entrance_queues = malloc(sizeof(queue_t)*ENTRANCE_COUNT);
    initialize_queues(&entrance_queues, INITIAL_QUEUE_SIZE, ENTRANCE_COUNT);

    for (int i = 0; i < ENTRANCE_COUNT; i++) {
        entrance_args_t args;
        args.queue = entrance_queues[i];
        args.entrance = shm.data->entrance_collection[i];
        pthread_create(&entrance_threads[i], NULL, run_entrances, (void *)&args);
    }

    /* Car factory setup */
    car_args_t car_args;
    car_args.queue = entrance_queues;
    car_args.entrance_count = ENTRANCE_COUNT;
    car_args.rng_mutex = &rng_mutex;

    pthread_create(&car_factory_tid, NULL, generate_cars, (void *)&car_args);

    pthread_join(entrance_threads[0], NULL);
    return 0;
}