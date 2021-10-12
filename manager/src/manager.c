#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "shared_memory.h"
#include "plate_table.h"

#define SHARED_NAME "PARKING"
#define ENTRANCE_COUNT 5
#define BUCKETS 100

struct entrance_args {
    htab_t plates;
    entrance_t entrance;
};

/* Reads entrance input and checks against the list of accepted plates.*/
void* run_entrances(void *entrance_args) {
    struct entrance_args *args;
    args = (struct entrance_args*) entrance_args;
    
    pthread_mutex_lock(&args->entrance.lpr.mutex);
    
    while (true) {
        if (htab_find(&args->plates, args->entrance.lpr.license_plate) != NULL) {
            pthread_mutex_lock(&args->entrance.sign.mutex);
            // @TODO Boom gates, find appropriate level.
            args->entrance.sign.display = 1;
            pthread_cond_signal(&args->entrance.sign.cond);
            pthread_mutex_unlock(&args->entrance.sign.mutex);
            printf("Allowed access to %s\n", args->entrance.lpr.license_plate);
        } else {
            pthread_mutex_lock(&args->entrance.sign.mutex);
            args->entrance.sign.display = 'D';
            pthread_cond_signal(&args->entrance.sign.cond);
            pthread_mutex_unlock(&args->entrance.sign.mutex);
            printf("Denied access to %s\n", args->entrance.lpr.license_plate);
        }
        pthread_cond_wait(&args->entrance.lpr.cond, &args->entrance.lpr.mutex);
    }
    pthread_exit(NULL);
}

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
        char *plate = malloc(sizeof(char)*7);
        memcpy(plate, line, 7);
        if (htab_add(&plates, plate) == false) {
            printf("Failed to add item to hash table.\n");
            return EXIT_FAILURE;
        }
    }

    pthread_t entrance_threads[ENTRANCE_COUNT];
    struct entrance_args args[ENTRANCE_COUNT];

    // create enough threads to handle each entrance
    for (int i = 0; i < ENTRANCE_COUNT; i++) {
        args[i].entrance = shm.data->entrance_collection[i];
        args[i].plates = plates;
        pthread_create(&entrance_threads[i], NULL, run_entrances, (void *)&args[i]);
    }

    pthread_join(entrance_threads[0], NULL);

    return 0;
}