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
    level_info_t **level_info;
};

typedef struct level_args {
    level_info_t *level;
    lpr_t *lpr;
} level_args_t;

/* Plates aren't null terminated so no strcmp */
bool plate_compare(char plate1[6], char plate2[6]) {
    for (int i = 0; i < 6; i++) {
        if (plate1[i] != plate2[i]) {
            return false;
        }
    }
    return true;
}

/* If a matching plate is found on this level, the car is removed and true is returned.
    Otherwise malloc a car node and add the given plates to the end of the linked list. */
bool update_level(level_info_t *level_info, char plate[6]) {
    pthread_mutex_lock(&level_info->mutex);
    car_node_t *curr = level_info->head;
    car_node_t *prev = NULL;

    // Special case, head contains the plate that is looking for.
    if (curr != NULL && plate_compare(curr->car.plate, plate)) {
        level_info->head = curr->next;
        free(curr);
        pthread_mutex_unlock(&level_info->mutex);
        return true;
    } else {
        while (curr != NULL && !plate_compare(curr->car.plate, plate)) {
            prev = curr;
            curr = curr->next;
        }
        if (curr == NULL) {
            /* Car was not found. Allocate memory for a car and return false. */
            car_node_t *node = malloc(sizeof(car_node_t));
            memcpy(node->car.plate, plate, 6);
            node->next = level_info->head;
            level_info->head = node;
            pthread_mutex_unlock(&level_info->mutex);
            return false;
        } else {
            prev->next = curr->next;
            free(curr);
            pthread_mutex_unlock(&level_info->mutex);
            return true;
        }
    }
}

/* Gets the level that the current car should go to. Returns -1 if park is full. */
int get_level(level_info_t **levels) {
    for (int i = 0; i < LEVEL_COUNT; i++) {
        pthread_mutex_lock(&levels[i]->mutex);
        if (levels[i]->current_cars <  CARS_PER_LEVEL) {
            pthread_mutex_unlock(&levels[i]->mutex);
            return i + 1;
        }
        pthread_mutex_unlock(&levels[i]->mutex);
    }
    return -1;
}

/* Handles incoming and outgoing cars per level. One thread per level. */
void *run_level(void *level_args) {
    level_args_t *args;
    args = (level_args_t *) level_args;

    char empty_plate[6];
    memset(empty_plate, 0, sizeof(empty_plate));

    while (true) {
        pthread_mutex_lock(&args->lpr->mutex);

        pthread_cond_wait(&args->lpr->cond, &args->lpr->mutex);

        if (!plate_compare(empty_plate, args->lpr->license_plate)) {
            /* Got valid plates */
            bool result = update_level(args->level, args->lpr->license_plate);
            pthread_mutex_unlock(&args->lpr->mutex);
            pthread_mutex_lock(&args->level->mutex);
            if (result) {
                args->level->current_cars--;
            } else {
                args->level->current_cars++;
            }
            pthread_mutex_unlock(&args->level->mutex);
        } else {
            pthread_mutex_unlock(&args->lpr->mutex);
        }
    }
    pthread_exit(NULL);
}

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
            int level = get_level(args->level_info);
            if (level == -1) {
                update_sign(&args->entrance->sign, 'F');
            } else {
                char level_char = level + '0';
                update_sign(&args->entrance->sign, level_char);

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
            }
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

    pthread_t levels_tid[LEVEL_COUNT];
    level_args_t level_args[LEVEL_COUNT];
    for (int i = 0; i < LEVEL_COUNT; i++) {
        level_args[i].level = levels[i];
        level_args[i].lpr = &shm.data->level_collection[i].lpr;
        pthread_create(&levels_tid[i], NULL, run_level, (void *)&level_args[i]);
    }

    float revenue = 0;

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
        args[i].level_info = levels;
        pthread_create(&entrance_threads[i], NULL, run_entrances, (void *)&args[i]);
    }

    //@todo
    pthread_join(entrance_threads[0], NULL);

    return 0;
}