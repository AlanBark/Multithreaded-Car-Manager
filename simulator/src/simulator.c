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

    

    return 0;
}