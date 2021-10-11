#include "sign.h"
#include <string.h>
#include <pthread.h>

void initialize_sign(sign_t *sign) {

    pthread_mutexattr_t mutexAttr;
    pthread_mutexattr_init(&mutexAttr);
    pthread_mutexattr_setpshared(&mutexAttr, 1);
    pthread_mutex_init(&sign->mutex, &mutexAttr);

    pthread_condattr_t condAttr;
    pthread_condattr_init(&condAttr);
    pthread_condattr_setpshared(&condAttr, 1);
    pthread_cond_init(&sign->cond, &condAttr);
}

void update_sign(sign_t* sign, char display) {
    
    pthread_mutex_lock(&sign->mutex);
    sign->display = display;
    pthread_cond_signal(&sign->cond);
    pthread_mutex_unlock(&sign->mutex);

}