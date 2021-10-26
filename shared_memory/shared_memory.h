#pragma once
#include <stdbool.h>

#include "entrance.h"
#include "exit.h"
#include "level.h"

#define LEVEL_COUNT 10
#define ENTRANCE_COUNT 10
#define EXIT_COUNT 10

// Constants control how many are actually initialized.
typedef struct shared_data {

    entrance_t entrance_collection[ENTRANCE_COUNT];

    exit_t exit_collection[EXIT_COUNT];

    level_t level_collection[LEVEL_COUNT];

} shared_data_t;


typedef struct shared_memory {

    const char* name;

    int fd;

    shared_data_t* data;

} shared_memory_t;

bool create_shared_object(shared_memory_t* shm, const char* share_name, bool create);

bool initialize_shared_object(shared_memory_t *shm, int num_entrances, int num_exits, int num_levels);
