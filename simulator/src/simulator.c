#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "entrance.h"
#include "exit.h"
#include "level.h"
#include "shared_memory.h"
#include "util.h"

#define SHARED_NAME "PARKING"

// Accepted range 1-5 inclusive

#define LEVEL_COUNT 5

#define ENTRANCE_COUNT 5

#define EXIT_COUNT 5

void generate_cars(int entrance_count,char plate[7], pthread_mutex_t *rng_mutex) {

    plate[0] = get_random_number(rng_mutex, 48, 57);
    plate[1] = get_random_number(rng_mutex, 48, 57);
    plate[2] = get_random_number(rng_mutex, 48, 57);
    
    plate[3] = get_random_number(rng_mutex, 65, 90);
    plate[4] = get_random_number(rng_mutex, 65, 90);
    plate[5] = get_random_number(rng_mutex, 65, 90);
    plate[6] = 0;
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
    char plate[7];
    generate_cars(ENTRANCE_COUNT, plate, &rng_mutex);
    printf("%s\n", plate);
    return 0;
}