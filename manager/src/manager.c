#include <stdio.h>

#include "shared_memory.h"

#define SHARED_NAME "PARKING"


int main(int argc, char **argv) {

    shared_memory_t shm;

    // attempt to open shared memory
    if (!create_shared_object(&shm, SHARED_NAME, false)) {
        fprintf(stderr, "ERROR Failed to open existing shared memory\n");
        return 1;
    }

    return 0;
}