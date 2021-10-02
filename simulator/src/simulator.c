#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include "entrance.h"
#include "exit.h"
#include "level.h"
#include "shared_memory.h"

#define SHARED_NAME "PARKING"

// Accepted range 1-5 inclusive

#define LEVEL_COUNT 5

#define ENTRANCE_COUNT 5

#define EXIT_COUNT 5

int main(int argc, char **argv) {
    shared_memory_t shm;
    
    // Remove previous instances of shared memory
    shm_unlink(SHARED_NAME);

    shm.name = SHARED_NAME;

    shm.fd = shm_open(SHARED_NAME, O_RDWR | O_CREAT , S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if (shm.fd < 0) {
        shm.data = NULL;
        return false;
    }

    // Set the capacity of the shared memory object via ftruncate. If the 
    // operation fails, ensure that shm->data is NULL and return false. 
    if (ftruncate(shm.fd, sizeof(shared_data_t)) < 0) {
        shm.data = NULL;
        return false;
    }

    // Otherwise, attempt to map the shared memory via mmap, and save the address
    // in shm->data. If mapping fails, return false.
    shm.data = mmap(NULL, sizeof(shared_data_t), PROT_READ | PROT_WRITE, MAP_SHARED ,shm.fd, 0);
    if (shm.data == MAP_FAILED) {
        return false;
    }

    return 0;
}