#include <stdbool.h>

#include "entrance.h"
#include "exit.h"
#include "level.h"

// Always reserve 2920 byes (5 of each).
// Constants control how many are actually initialized.
typedef struct shared_data {

    entrance_t entrance_collection[5];

    exit_t exit_collection[5];

    level_t level_collection[5];

} shared_data_t;


typedef struct shared_memory {

    const char* name;

    int fd;

    shared_data_t* data;

} shared_memory_t;

bool create_shared_object(shared_memory_t* shm, const char* share_name);

bool initialize_shared_object(shared_memory_t *shm, int num_entrances, int num_exits, int num_levels);
