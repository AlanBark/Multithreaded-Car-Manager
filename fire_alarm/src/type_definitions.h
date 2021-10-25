#include <pthread.h>

typedef struct gate {

    pthread_mutex_t mutex;

    pthread_cond_t cond;

    char status;

} gate_t;

typedef struct sign {

    pthread_mutex_t mutex;

    pthread_cond_t cond;

    char display;

} sign_t;

typedef struct lpr {

    pthread_mutex_t mutex;

    pthread_cond_t cond;

    char license_plate[6];

} lpr_t;

typedef struct level {

    lpr_t lpr;

    volatile int16_t sensor;

    volatile int alarm;

} level_t;

typedef struct entrance {

    lpr_t lpr;

    gate_t gate;

    sign_t sign;

} entrance_t;

typedef struct exit {

    lpr_t lpr;

    gate_t gate;

} exit_t;

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