#include "entrance.h"
#include "exit.h"
#include "level.h"

// Always reserve 2920 byes (5 of each).
// Constants control how many are initialized.
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
