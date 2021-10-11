#include <stdlib.h>
#include "util.h"

/* Utility functions used in simulator */

/* Sleeps thread for a specified amount of millliseconds */
void ms_sleep(long msec) {
    msec = msec * 10; //@TODO todo tag so i remember to remove this
    struct timespec ts;
    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;
    nanosleep(&ts, &ts);
}

int get_random_number(pthread_mutex_t *mutex, int minimum, int maximum) {
    pthread_mutex_lock(mutex);
    int number = rand();
    pthread_mutex_unlock(mutex);
    return (number % (maximum - minimum + 1) + minimum);
}