#include <pthread.h>

// Struct of a car from manager POV.
// Note struct car also exists from simulator POV.
typedef struct car {
    char plate[6];
    time_t time_entered;
} car_t;

typedef struct level_info {
    int current_cars;
    car_t **cars;
    pthread_mutex_t mutex;
} level_info_t;

void update(int num_entrances, int num_exits, int num_levels, int cars_per_level, shared_data_t *data, level_info_t **level_info, float revenue);

void initialise_level_info(level_info_t *level_info, int max_cars);