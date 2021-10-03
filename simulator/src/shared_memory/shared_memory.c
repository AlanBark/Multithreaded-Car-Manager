#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include "shared_memory.h"

bool create_shared_object(shared_memory_t* shm, const char* share_name) {
    // Remove any previous instance of the shared memory object, if it exists.
    shm_unlink(share_name);

    // Assign share name to shm->name.
    shm->name = share_name;

    // Create the shared memory object, allowing read-write access, and saving the
    // resulting file descriptor in shm->fd. If creation failed, ensure 
    // that shm->data is NULL and return false.
    shm->fd = shm_open(share_name, O_RDWR | O_CREAT , S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if (shm->fd < 0) {
        shm->data = NULL;
        return false;
    }

    // Set the capacity of the shared memory object via ftruncate. If the 
    // operation fails, ensure that shm->data is NULL and return false. 
    if (ftruncate(shm->fd, sizeof(shared_data_t)) < 0) {
        shm->data = NULL;
        return false;
    }

    // Otherwise, attempt to map the shared memory via mmap, and save the address
    // in shm->data. If mapping fails, return false.
    shm->data = mmap(NULL, sizeof(shared_data_t), PROT_READ | PROT_WRITE, MAP_SHARED ,shm->fd, 0);
    if (shm->data == MAP_FAILED) {
        return false;
    }

    // If we reach this point we should return true.
    return true;
}

// @TODO change intialize from void to bool.
bool initialize_shared_object(shared_memory_t *shm, int num_entrances, int num_exits, int num_levels) {
    for (int i = 0; i < num_entrances; i++) {
        initialize_entrance(&shm->data->entrance_collection[i]);
    }
    for (int i = 0; i < num_exits; i++) {
        initialize_exit(&shm->data->exit_collection[i]);
    }
    for (int i = 0; i < num_levels; i++) {
        initialize_level(&shm->data->level_collection[i]);
    }
    return true;
}