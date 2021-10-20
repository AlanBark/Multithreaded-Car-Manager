#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include "shared_memory.h"
#include "plate_table.h"
#include "status.h"
#include "util.h"

#define SHARED_NAME "PARKING"

#define ENTRANCE_COUNT 5
#define EXIT_COUNT 5
#define LEVEL_COUNT 5
#define CARS_PER_LEVEL 20

#define BUCKETS 10000000

struct entrance_args {
    htab_t plates;
    entrance_t *entrance;
};

// Controls the lowering of boom gates after 20ms.
void *run_gates(void *gate_arg) {
    gate_t *gate;
    gate = (gate_t *)gate_arg;

    // Gate always alive
    while (true) {
        // block thread until gate is open
        pthread_mutex_lock(&gate->mutex);
        while (gate->status != 'O') {
            pthread_cond_wait(&gate->cond, &gate->mutex);
        }
        // release mutex for other threads, wait 20ms, take mutex and set gate to lowering.
        pthread_mutex_unlock(&gate->mutex);
        ms_sleep(20);
        update_gate(gate, 'L');
    }
    pthread_exit(NULL);
}

/* Reads entrance input and checks against the list of accepted plates.*/
void* run_entrances(void *entrance_args) {
    struct entrance_args *args;
    args = (struct entrance_args*) entrance_args;

    pthread_t gate_tid;
    pthread_create(&gate_tid, NULL, run_gates, (void*) &args->entrance->gate);
    
    // aquire mutex for duration of loop - unlocked by wait only
    pthread_mutex_lock(&args->entrance->lpr.mutex);
    while (true) {
        if (htab_find(&args->plates, args->entrance->lpr.license_plate) != NULL) {
            // car valid. Signal the car where to go and start boom gate process if closed.
            update_sign(&args->entrance->sign, '1');

            // Boom gate logic
            pthread_mutex_lock(&args->entrance->gate.mutex);
            if (args->entrance->gate.status == 'C') {
                args->entrance->gate.status = 'R';
                pthread_cond_broadcast(&args->entrance->gate.cond);
            } else {
                // Condition where gate is in lowering stage and must be re-opened
                while(args->entrance->gate.status == 'L') {
                    pthread_cond_wait(&args->entrance->gate.cond, &args->entrance->gate.mutex);
                }
                // after gate has changed from 'L', check again that is 'C'.
                if (args->entrance->gate.status == 'C') {
                    args->entrance->gate.status = 'R';
                    pthread_cond_broadcast(&args->entrance->gate.cond);
                }
            }
            pthread_mutex_unlock(&args->entrance->gate.mutex);


        } else {
            update_sign(&args->entrance->sign, 'X');
        }
        pthread_cond_wait(&args->entrance->lpr.cond, &args->entrance->lpr.mutex);
    }
    pthread_exit(NULL);
}

//
int main(int argc, char **argv) {

    shared_memory_t shm;

    // attempt to open shared memory
    if (!create_shared_object(&shm, SHARED_NAME, false)) {
        fprintf(stderr, "ERROR Failed to open existing shared memory\n");
        return EXIT_FAILURE;
    }

    /* Construct hash table from plates.txt*/

    htab_t plates;
    if (!htab_init(&plates, BUCKETS))
    {
        printf("failed to initialise hash table\n");
        return EXIT_FAILURE;
    }

    char *line;
    size_t line_len = 0;

    FILE *fp = fopen("plates.txt", "r");
    if (fp == NULL) {
        printf("Failed to open file plates.txt\n");
        return EXIT_FAILURE;
    }
    
    while (getline(&line, &line_len, fp) != -1) {
        // ignore last byte
        char *plate = malloc(sizeof(char)*6);
        memcpy(plate, line, 6);
        if (htab_add(&plates, plate) == false) {
            printf("Failed to add item to hash table.\n");
            return EXIT_FAILURE;
        }
    }

    level_info_t **levels = malloc(sizeof(level_info_t*) * LEVEL_COUNT);
    for (int i = 0; i < LEVEL_COUNT; i++) {
        levels[i] = malloc(sizeof(level_info_t));
        initialise_level_info(levels[i], CARS_PER_LEVEL);
    }

    float revenue = 12.23;

    status_args_t status_args;
    status_args.num_entrances = ENTRANCE_COUNT;
    status_args.num_exits = EXIT_COUNT;
    status_args.num_levels = LEVEL_COUNT;
    status_args.level_info = levels;
    status_args.cars_per_level = CARS_PER_LEVEL;
    status_args.data = shm.data;
    status_args.revenue = &revenue;
    pthread_t status_thread;

    pthread_create(&status_thread, NULL, update_status_display, (void*)&status_args);

    pthread_t entrance_threads[ENTRANCE_COUNT];
    struct entrance_args args[ENTRANCE_COUNT];

    // create enough threads to handle each entrance
    for (int i = 0; i < ENTRANCE_COUNT; i++) {
        args[i].entrance = &shm.data->entrance_collection[i];
        args[i].plates = plates;
        pthread_create(&entrance_threads[i], NULL, run_entrances, (void *)&args[i]);
    }

    //@todo
    pthread_join(entrance_threads[0], NULL);

    return 0;
}