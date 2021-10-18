#include "lpr.h"
#include <string.h>
#include <pthread.h>

void initialize_lpr(lpr_t* lpr) {
    memset(lpr->license_plate, 0, sizeof(lpr->license_plate));

    pthread_mutexattr_t mutexAttr;
    pthread_mutexattr_init(&mutexAttr);
    pthread_mutexattr_setpshared(&mutexAttr, 1);
    pthread_mutex_init(&lpr->mutex, &mutexAttr);

    pthread_condattr_t condAttr;
    pthread_condattr_init(&condAttr);
    pthread_condattr_setpshared(&condAttr, 1);
    pthread_cond_init(&lpr->cond, &condAttr);
}

void update_plate(lpr_t* lpr, char plate[6]) {
    pthread_mutex_lock(&lpr->mutex);
    memcpy(lpr->license_plate, plate, 6);
    pthread_cond_signal(&lpr->cond);
    pthread_mutex_unlock(&lpr->mutex);
}

/* get plate for display formatting */
void get_plate(lpr_t* lpr, char plate[7]) {
    pthread_mutex_lock(&lpr->mutex);
    memset(plate, 0, 7);
    if (lpr->license_plate[0] == '\0') {
        memcpy(plate, "------", 6);
    } else {
        memcpy(plate, lpr->license_plate, 6);
    }
    pthread_mutex_unlock(&lpr->mutex);
}