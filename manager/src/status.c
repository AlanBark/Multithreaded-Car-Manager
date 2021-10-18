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

/* Updates the status menu with the most recent hardware data.
Most of the below code is just a means to display pretty output
 */
void update(int num_entrances, int num_exits, int num_levels, int cars_per_level, shared_data_t *data, level_info_t **level_info, float revenue) {

    // approx number of chars display requires. Minimum 1040 if no levels or entrances. 
    // int display_size = 1200 + (num_entrances * 80) + (num_levels * 80);

    // char display[display_size];
    printf("* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\n\
* STATUS FOR ENTRANCES            STATUS FOR EXITS         REVENUE TOTAL      *\n\
* +-------------+------+------+   +-------------+------+   +----------------+ *\n\
* | ID | LPR    | BOOM | SIGN |   | ID | LPR    | BOOM |   | $%.2f\n\
* +----+--------+------+------+   +----+--------+------+   +----------------+ *\n", revenue);

    for (int i = 0; i < num_entrances; i++) {
        char plate[7];
        get_plate(&data->entrance_collection[i].lpr, plate);
        char gate = get_gate(&data->entrance_collection[i].gate);
        char sign = get_sign(&data->entrance_collection[i].sign);
        if (i < num_exits) {
            char exit_gate = get_gate(&data->exit_collection[i].gate);
            char exit_plate[7];
            get_plate(&data->exit_collection[i].lpr, exit_plate);
            printf("* | %d  | %s | %c    | %c    |   | %d  | %s | %c    |                      *\n", i+1, plate, gate, sign, i, exit_plate, exit_gate);
        } else {
            printf("* | %d  | %s | %c    | %c    |                                               *\n", i+1, plate, gate, sign);
        }
    }

    printf("* +----+--------+------+------+   +----+--------+------+                      *\n\
*                                                                             *\n\
* STATUS FOR LEVELS                                                           *\n\
* +-------+--------+------+----------+----------+                             *\n\
* | LEVEL | LPR    | CARS | CARS MAX | TEMP (C) |   '-' = No Values Found     *\n\
* +-------+--------+------+----------+----------+                             *\n");

    for (int i = 0; i < num_levels; i++) {
        char plate[7];
        get_plate(&data->level_collection[i].lpr, plate);
        int current_cars = get_current_cars(level_info[i]);
        printf("* | %d     | %s | %d    | %d       | %d        |                             *\n", i+1, plate, current_cars, cars_per_level, data->level_collection[i].sensor);
    }

    printf("* +-------+--------+------+----------+----------+                             *\n\
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\n");
}

void initialise_level_info(level_info_t *level_info, int max_cars) {
    pthread_mutex_init(&level_info->mutex, NULL);
    level_info->current_cars = 0;
    level_info->cars = malloc(sizeof(car_t) * max_cars);
}