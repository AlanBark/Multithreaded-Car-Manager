#include <pthread.h>
#include <time.h>
#include <unistd.h>

/* Sleeps thread for a specified amount of millliseconds */
void ms_sleep(long msec);

long get_current_time_ms();

int get_random_number(pthread_mutex_t *mutex, int minimum, int maximum);