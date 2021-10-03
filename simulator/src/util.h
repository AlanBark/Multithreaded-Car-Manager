#include <pthread.h>
#include <time.h>
#include <unistd.h>

/* Sleeps thread for a specified amount of millliseconds */
void ms_sleep(long msec);

int get_random_number(pthread_mutex_t *mutex, int minimum, int maximum);

void generate_cars(int entrance_count);