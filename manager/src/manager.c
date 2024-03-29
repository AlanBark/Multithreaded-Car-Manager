#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include "shared_memory.h"
#include "plate_table.h"
#include "status.h"
#include "util.h"

/* LEVELS, EXITS, ENTRANCES defined in shared_memory.h */

#define SHARED_NAME "PARKING"

#define BILLING_FILE "Billing.txt"

#define CARS_PER_LEVEL 20

/* Change reasonably based on size of plates.txt */
#define BUCKETS 10000

/* Argument structs for exit threads */
typedef struct exit_args {
    revenue_info_t *revenue_info;
    exit_t *exit;
    pthread_mutex_t *file_mutex;
    volatile int *alarm;
} exit_args_t;

struct entrance_args {
    htab_t plates;
    entrance_t *entrance;
    level_info_t **level_info;
    revenue_info_t *revenue_info;
    volatile int *alarm;
};

typedef struct level_args {
    level_info_t *level;
    lpr_t *lpr;
} level_args_t;

typedef struct gate_args {
    gate_t *gate;
    volatile int *alarm;
} gate_args_t;

/* Plates aren't null terminated so no strcmp */
bool plate_compare(char plate1[6], char plate2[6]) {
    for (int i = 0; i < 6; i++) {
        if (plate1[i] != plate2[i]) {
            return false;
        }
    }
    return true;
}

/* 
 * Function update_level(): Update the internal list of cars for a level.
 * 
 * Algorithm: If a matching plate is found on this level, the car is removed 
 *            from the internal listand true is returned. Otherwise, add the
 *            car to the internal list and return false.
 * Input:     Level_info object for the level to be updating. License plates of
 *            Car to add/remove.
 * Output:    True if car is removed, false if car is added.
 * 
 * Pre:       initialise_level_info(level_info);
 */
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

/* 
 * Function revenue_info_add(): Add a car to the managers revenue info struct
 * 
 * Algorithm: Create a car object with the time_entered value set to the current
 *            Time.
 * Input:     Manager's revenue info struct, plates of car to add.
 * Output:    None
 *  
 * Pre:       pthread_mutex_init(info->mutex)
 */
void revenue_info_add(revenue_info_t *info, char plate[6]) {
    pthread_mutex_lock(&info->mutex);
    car_node_t *node = malloc(sizeof(car_node_t));
    memcpy(node->car.plate, plate, 6);
    node->car.time_entered = get_current_time_ms();
    if (info->head == NULL) {
        info->head = node;
    } else {
        node->next = info->head;
        info->head = node;
    }
    pthread_mutex_unlock(&info->mutex);
}

/* 
 * Function calculate_revenue(): Calculates the amount of money a car pays when leaving
 * 
 * Consumer to revenue_info_add()
 * 
 * Algorithm: Removes car from revenue list, updates revenue total, returns cost to 
 *            The current car.
 * Input:     Manager's revenue info struct, plates of car to add.
 * Output:    Cost to the current car.
 *  
 * Pre:       revenue_info_add(plates[6]) has been called.
 */
int calculate_revenue(revenue_info_t *info, char plate[6]) {

    pthread_mutex_lock(&info->mutex);
    car_node_t *curr = info->head;
    car_node_t *prev = NULL;
    
    long current_time_ms = get_current_time_ms();
    int cost, time_spent_parked;

    // Special case, head contains the plate that is looking for.
    if (curr != NULL && plate_compare(curr->car.plate, plate)) {
        time_spent_parked = current_time_ms - curr->car.time_entered;
        cost = time_spent_parked * 5;
        info->revenue += cost;
        info->head = curr->next;
        free(curr);
    } else {
        while (curr != NULL && !plate_compare(curr->car.plate, plate)) {
            prev = curr;
            curr = curr->next;
        }
        if (curr != NULL) {
            time_spent_parked = current_time_ms - curr->car.time_entered;
            cost = time_spent_parked * 5;
            info->revenue += cost;
            prev->next = curr->next;
            free(curr);
        }
    }
    pthread_mutex_unlock(&info->mutex);
    return cost;
}


/* 
 * Function get_level(): Determines which level the next car should go to.
 * 
 * Algorithm: Searches each level in ascending order and returns the level number
 *            With the first available parking space.
 * Input:     Array of level_info pointers.
 * Output:    Level car should go to. Returns -1 if full.
 *  
 * Pre:       All levels have been initialised.
 */
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

/* 
 * Function run_level(): Logic loop for each level thread.
 * 
 * Algorithm: Wait for non-empty plates on the level LPR. Call
 *            update_level() on the plates received, and either increment
 *            or decrement the number of cars currently on the level.
 * Input:     A level_args_t pointer cast to a void*
 * Output:    None
 */
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

/* 
 * Function run_gates(): Logic loop for each boom gates thread.
 * 
 * Algorithm: Waits for gate to enter 'O' state. Sleeps for 20ms
 *            then sets the gate to 'L' state.
 * Input:     A gate_t pointer cast to a void*
 * Output:    None
 */
void *run_gates(void *gate_arg) {
    gate_args_t *args;
    args = (gate_args_t *)gate_arg;
    gate_t *gate = args->gate;

    // Gate always alive
    while (*args->alarm == 0) {
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

/* 
 * Function run_entrances(): Logic loop for each entrance thread.
 * 
 * Algorithm: Creates a boom gate thread. Waits for a new plate to come from
 *            The entrance LPR. If this plate is not listed in plates.txt,
 *            Show the car an X and continue waiting. Otherwise, get and
 *            display the level that the car should go to and set boom gates
 *            to 'R' if 'C' or wait if 'L'
 * Input:     An entrance_args_t pointer cast to a void*
 * Output:    None
 */
void* run_entrances(void *entrance_args) {
    struct entrance_args *args;
    args = (struct entrance_args*) entrance_args;

    pthread_t gate_tid;
    gate_args_t gate_args;
    gate_args.gate = &args->entrance->gate;
    gate_args.alarm = args->alarm;
    pthread_create(&gate_tid, NULL, run_gates, (void*)&gate_args);
    
    // aquire mutex for duration of loop - unlocked by wait only
    pthread_mutex_lock(&args->entrance->lpr.mutex);
    while (true) {
        if (*args->alarm == 0) {
            if (htab_find(&args->plates, args->entrance->lpr.license_plate) != NULL) {
                // car valid. Signal the car where to go and start boom gate process if closed.
                int level = get_level(args->level_info);
                if (level == -1) {
                    update_sign(&args->entrance->sign, 'F');
                } else {
                    char level_char = level + '0';
                    update_sign(&args->entrance->sign, level_char);

                    revenue_info_add(args->revenue_info, args->entrance->lpr.license_plate);

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
        } else {
            pthread_cond_wait(&args->entrance->lpr.cond, &args->entrance->lpr.mutex);
        }
    }
    pthread_exit(NULL);
}

/* 
 * Function run_exits(): Logic loop for each exit thread.
 * 
 * Algorithm: Waits for non null plate to show in LPR. Calculates revenue
 *            From each car and writes to an output file. Controls boom gates
 *            using the same logic as run_entrances.
 * Input:     An exit_args_t pointer cast to a void*
 * Output:    None
 */
void *run_exits(void *exit_args) {
    exit_args_t *args;
    args = (exit_args_t*) exit_args;
    revenue_info_t *info;
    info = args->revenue_info;
    exit_t *exit;
    exit = args->exit;

    pthread_t gate_tid;
    gate_args_t gate_args;
    gate_args.gate = &args->exit->gate;
    gate_args.alarm = args->alarm;
    pthread_create(&gate_tid, NULL, run_gates, (void*)&gate_args);

    char empty_plate[6];
    memset(empty_plate, 0, sizeof(empty_plate));
    
    while (true) {
        pthread_mutex_lock(&exit->lpr.mutex);
        
        pthread_cond_wait(&exit->lpr.cond, &exit->lpr.mutex);
        
        if (!plate_compare(exit->lpr.license_plate, empty_plate)) {
            // plate used to write to billing file. Needs to be null terminated.
            char plate[7];
            int cost = calculate_revenue(info, exit->lpr.license_plate);
            memcpy(plate, exit->lpr.license_plate, 6);
            plate[7] = '\0';
            pthread_mutex_unlock(&exit->lpr.mutex);

            // Boom gate logic
            pthread_mutex_lock(&exit->gate.mutex);
            if (exit->gate.status == 'C') {
                exit->gate.status = 'R';
                pthread_cond_broadcast(&exit->gate.cond);
            } else {
                // Condition where gate is in lowering stage and must be re-opened
                while(exit->gate.status == 'L') {
                    pthread_cond_wait(&exit->gate.cond, &exit->gate.mutex);
                }
                // after gate has changed from 'L', check again that is 'C'.
                if (exit->gate.status == 'C') {
                    exit->gate.status = 'R';
                    pthread_cond_broadcast(&exit->gate.cond);
                }
            }
            pthread_mutex_unlock(&exit->gate.mutex);

            int dollars = cost / 100;
            int cents = cost % 100;
            
            pthread_mutex_lock(args->file_mutex);
            FILE *fp = fopen(BILLING_FILE, "a");

            fprintf(fp, "%s $%d.%d\n",plate, dollars, cents);

            fclose(fp);

            pthread_mutex_unlock(args->file_mutex);
        }
    }
    pthread_exit(NULL);
}

/* Manager program capable of directing cars in a variable sized carpark */
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
    
    /* Read plates.txt into a hash table */
    while (getline(&line, &line_len, fp) != -1) {
        // ignore last byte
        char *plate = malloc(sizeof(char)*6);
        memcpy(plate, line, 6);
        if (htab_add(&plates, plate) == false) {
            printf("Failed to add item to hash table.\n");
            return EXIT_FAILURE;
        }
    }

    /* Threads to be created by manager */
    pthread_t exit_threads[EXIT_COUNT];
    pthread_t entrance_threads[ENTRANCE_COUNT];
    pthread_t levels_tid[LEVEL_COUNT];
    pthread_t status_thread;

    level_info_t **levels = malloc(sizeof(level_info_t*) * LEVEL_COUNT);
    for (int i = 0; i < LEVEL_COUNT; i++) {
        levels[i] = malloc(sizeof(level_info_t));
        initialise_level_info(levels[i], CARS_PER_LEVEL);
    }

    revenue_info_t revenue;
    pthread_mutex_init(&revenue.mutex, NULL);
    revenue.head = NULL;
    revenue.revenue = 0;
    
    level_args_t level_args[LEVEL_COUNT];

    status_args_t status_args;
    status_args.num_entrances = ENTRANCE_COUNT;
    status_args.num_exits = EXIT_COUNT;
    status_args.num_levels = LEVEL_COUNT;
    status_args.level_info = levels;
    status_args.cars_per_level = CARS_PER_LEVEL;
    status_args.data = shm.data;
    status_args.revenue = &revenue;

    
    struct entrance_args entrance_args[ENTRANCE_COUNT];
    exit_args_t exit_args[EXIT_COUNT];
    
    for (int i = 0; i < LEVEL_COUNT; i++) {
        level_args[i].level = levels[i];
        level_args[i].lpr = &shm.data->level_collection[i].lpr;
        pthread_create(&levels_tid[i], NULL, run_level, (void *)&level_args[i]);
    }
    
    pthread_create(&status_thread, NULL, update_status_display, (void*)&status_args);

    pthread_mutex_t file_mutex;
    pthread_mutex_init(&file_mutex, NULL);

    // create enough threads to handle each exit
    for (int i = 0; i < EXIT_COUNT; i++) {
        exit_args[i].exit = &shm.data->exit_collection[i];
        exit_args[i].revenue_info = &revenue;
        exit_args[i].file_mutex = &file_mutex;
        exit_args[i].alarm = &shm.data->level_collection[0].alarm;
        pthread_create(&exit_threads[i], NULL, run_exits, (void *)&exit_args[i]);
    }

    // create enough threads to handle each entrance
    for (int i = 0; i < ENTRANCE_COUNT; i++) {
        entrance_args[i].entrance = &shm.data->entrance_collection[i];
        entrance_args[i].plates = plates;
        entrance_args[i].level_info = levels;
        entrance_args[i].revenue_info = &revenue;
        entrance_args[i].alarm = &shm.data->level_collection[0].alarm;
        pthread_create(&entrance_threads[i], NULL, run_entrances, (void *)&entrance_args[i]);
    }

    for (int i = 0; i < ENTRANCE_COUNT; i++) {
        pthread_join(entrance_threads[i], NULL);
    }

    for (int i = 0; i < EXIT_COUNT; i++) {
        pthread_join(exit_threads[i], NULL);
    }

    for (int i = 0; i < LEVEL_COUNT; i++) {
        pthread_join(levels_tid[i], NULL);
    }

    pthread_join(status_thread, NULL);

    return 0;
}