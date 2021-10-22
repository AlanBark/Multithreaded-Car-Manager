#include <inttypes.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include "util.h"

/* Utility functions used in simulator */

/* Sleeps thread for a specified amount of millliseconds */
void ms_sleep(long msec) {
    msec = msec * 100; //@TODO todo tag so i remember to remove this
    struct timespec ts;
    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;
    nanosleep(&ts, &ts);
}

long get_current_time_ms()
{
    long ms;
    struct timespec spec;

    clock_gettime(CLOCK_REALTIME, &spec);

    ms = round(spec.tv_nsec / 1000000);
    if (ms > 999) {
        spec.tv_sec++;
        ms = 0;
    }
    ms = ms + (spec.tv_sec * 1000);
    return ms;
}

int get_random_number(pthread_mutex_t *mutex, int minimum, int maximum) {
    pthread_mutex_lock(mutex);
    int number = rand();
    pthread_mutex_unlock(mutex);
    return (number % (maximum - minimum + 1) + minimum);
}