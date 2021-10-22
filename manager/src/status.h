#include <pthread.h>

// Struct of a car from manager POV.
// Note struct car also exists from simulator POV.
typedef struct car {
    char plate[6];
    time_t time_entered;
} car_t;

/* Car object for level info */
typedef struct car_node {
    car_t car;
    struct car_node *next;
} car_node_t;

/* Hold information that the manager sees about each level */
typedef struct level_info {
    int current_cars;
    car_node_t *head;
    pthread_mutex_t mutex;
} level_info_t;

typedef struct status_args {
    int num_entrances;
    int num_exits;
    int num_levels;
    int cars_per_level;
    shared_data_t* data;
    level_info_t **level_info;
    float *revenue;
} status_args_t;

void *update_status_display(void *status_args);

void *run_status(void *status_args_t);

void initialise_level_info(level_info_t *level_info, int max_cars);