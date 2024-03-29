#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <pthread.h>

#include "shared_memory.h"
#include "status.h"
#include "util.h"

int get_current_cars(level_info_t *level_info) {
    pthread_mutex_lock(&level_info->mutex);
    int cars = level_info->current_cars;
    pthread_mutex_unlock(&level_info->mutex);
    return cars;
}

/* 
 * Function update_status_display(): Logic loop for the status display thread.
 * 
 * Algorithm: Updates the status menu with the most recent hardware data.
 *            Constructs a single large string before printing to reduce screen flicker.
 *            Instead of system("clear"), the program moves the cursor to the top left of the terminal
 *            and overwrites. This won't work on every terminal, tested working on CAB403 VM and windows terminal.
 * Input:     A status_args_t object cast to a void*
 * Output:    None
 */
void *update_status_display(void *status_args) {
    status_args_t *args;
    args = (status_args_t*) status_args;

    shared_data_t *data = args->data;
    int num_entrances = args->num_entrances;
    int num_levels = args->num_levels;
    int num_exits = args->num_exits;
    int cars_per_level = args->cars_per_level;
    level_info_t **level_info = args->level_info;
    revenue_info_t *revenue = args->revenue;

    while (true) {
        int length = 0;
        int buffer_len = 1400 + (80 * num_entrances) + (80 * num_levels);
        char buffer[buffer_len];

        pthread_mutex_lock(&revenue->mutex);
        float revenue_dollars = revenue->revenue / 100.0;
        pthread_mutex_unlock(&revenue->mutex);

    length += sprintf(buffer+length, "* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\n\
* STATUS FOR ENTRANCES            STATUS FOR EXITS         REVENUE TOTAL      *\n\
* +-------------+------+------+   +-------------+------+   +----------------+ *\n\
* | ID | LPR    | BOOM | SIGN |   | ID | LPR    | BOOM |   | $%.2f\n\
* +----+--------+------+------+   +----+--------+------+   +----------------+ *\n", revenue_dollars);

    for (int i = 0; i < num_entrances; i++) {
        char plate[7];
        get_plate(&data->entrance_collection[i].lpr, plate);
        char gate = get_gate(&data->entrance_collection[i].gate);
        char sign = get_sign(&data->entrance_collection[i].sign);
        if (i < num_exits) {
            char exit_gate = get_gate(&data->exit_collection[i].gate);
            char exit_plate[7];
            get_plate(&data->exit_collection[i].lpr, exit_plate);
            length += sprintf(buffer+length, "* | %d  | %s | %c    | %c    |   | %d  | %s | %c    |                      *\n", i+1, plate, gate, sign, i+1, exit_plate, exit_gate);
        } else {
            length += sprintf(buffer+length, "* | %d  | %s | %c    | %c    |                                               *\n", i+1, plate, gate, sign);
        }
    }

    length += sprintf(buffer+length, "* +----+--------+------+------+   +----+--------+------+                      *\n\
*                                                                             *\n\
* STATUS FOR LEVELS                                                           *\n\
* +-------+--------+------+----------+----------+                             *\n\
* | LEVEL | LPR    | CARS | CARS MAX | TEMP (C) |   '-' = No Values Found     *\n\
* +-------+--------+------+----------+----------+                             *\n");

    for (int i = 0; i < num_levels; i++) {
        char plate[7];
        get_plate(&data->level_collection[i].lpr, plate);
        int current_cars = get_current_cars(level_info[i]);
        if (current_cars > 9) {
            length += sprintf(buffer+length, "* | %d     | %s | %d   | %d       | %d       |                             *\n", i+1, plate, current_cars, cars_per_level, data->level_collection[i].sensor);
        } else {
            length += sprintf(buffer+length, "* | %d     | %s | %d    | %d       | %d       |                             *\n", i+1, plate, current_cars, cars_per_level, data->level_collection[i].sensor);
        }
    }

    length += sprintf(buffer+length, "* +-------+--------+------+----------+----------+                             *\n\
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\n");
        // reset cursor to top left of screen
        // not as portable as system("clear") but doesn't flicker
        printf("\033[2J\033[1;1H");
        printf(buffer);
        ms_sleep(50);
    }
}

void initialise_level_info(level_info_t *level_info, int max_cars) {
    pthread_mutex_init(&level_info->mutex, NULL);
    level_info->current_cars = 0;
    level_info->head = NULL;
}