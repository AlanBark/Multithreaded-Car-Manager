#include <stdlib.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#include "type_definitions.h"

#define SHARED_NAME "PARKING"

#define LEVELS 5
#define ENTRANCES 5
#define EXITS 5

#define MEDIAN_WINDOW 5
#define TEMPCHANGE_WINDOW 30

struct boomgate {
	pthread_mutex_t m;
	pthread_cond_t c;
	char s;
};

struct parkingsign {
	pthread_mutex_t m;
	pthread_cond_t c;
	char display;
};

struct monitor_args {
	int level;
	volatile int *alarm_active;
	shared_data_t *data;
};

/* Sleeps thread for a specified amount of millliseconds */
static void ms_sleep(long msec) {
    struct timespec ts;
    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;
    nanosleep(&ts, &ts);
}

static void *tempmonitor(void *thread_args)
{
	struct monitor_args *args;
	args = (struct monitor_args *)thread_args;
	int level = args->level;
	volatile int *alarm_active = args->alarm_active;
	shared_data_t *data = args->data;

	int temp;
	int hightemps;

	int16_t temp_list[5];
	int16_t median_list[30];

	for (int i = 0; i < MEDIAN_WINDOW; i++) {
		temp_list[i] = -999;
	}
	for (int i = 0; i < TEMPCHANGE_WINDOW; i++) {
		median_list[i] = -999;
	}

	for (;;) {
		temp = data->level_collection[level].sensor;
		int i = (int)MEDIAN_WINDOW - 1;
		for (i; i > 0; i--) {
			int j = i - 1;
			temp_list[i] = temp_list[j];
		}
		temp_list[0] = temp;

		if (temp_list[MEDIAN_WINDOW - 1] != -999) { // Temperatures are only counted once we have 5 samples

			int temp_list_copy[5];

			/* Copy temperature list then sort the copy */
			for (int i = 0; i < MEDIAN_WINDOW; i++) {
				temp_list_copy[i] = temp_list[i];
			}

			/* Insert sort */
			for (int i = 1; i < MEDIAN_WINDOW; i++) {
				int key = temp_list_copy[i];
				int j = i - 1;
				while ((j >= 0) && (temp_list_copy[j] > key)) {
					temp_list_copy[j + 1] = temp_list_copy[j];
					j = j - 1;
				}	
				temp_list_copy[j + 1] = key;
			}

			int median = temp_list_copy[(MEDIAN_WINDOW - 1) / 2];
			/* Add median to front of median list, discard last value */
			int i = (int)TEMPCHANGE_WINDOW - 1;
			for (i; i > 0; i--) {
				int j = i - 1;
				median_list[i] = median_list[j];
			}
			median_list[0] = median;
			
			hightemps = 0;
			
			for (int i = 0; i < TEMPCHANGE_WINDOW; i++) {
				// Temperatures of 58 degrees and higher are a concern
				if (median_list[i] >= 58) {
					hightemps++;
				}
				// Store the oldest temperature for rate-of-rise detection
			}
			
			/* If the oldest median value has been set, 30 values have been entered */
			if (median_list[TEMPCHANGE_WINDOW - 1] != -999) {
				// If 90% of the last 30 temperatures are >= 58 degrees,
				// this is considered a high temperature. Raise the alarm
				int highttemps_90 = (int)(TEMPCHANGE_WINDOW * 0.9);
				if (hightemps >= highttemps_90) {
					*alarm_active = 1;
				}
				
				// If the newest temp is >= 8 degrees higher than the oldest
				// temp (out of the last 30), this is a high rate-of-rise.
				// Raise the alarm
				int last_index = (int)TEMPCHANGE_WINDOW - 1;
				int temp_diff = (int)(median_list[TEMPCHANGE_WINDOW - 1] - median_list[0]);
				if (temp_diff >= 8) {
					*alarm_active = 1;
				}
			}
		}
		ms_sleep(2);
	}
	pthread_exit(NULL);
}

static void *openboomgate(void *arg)
{
	gate_t *bg;
	bg = (gate_t *)arg;

	pthread_mutex_lock(&bg->mutex);
	for (;;) {
		if (bg->status == (char)'C') {
			bg->status = 'R';
			pthread_cond_broadcast(&bg->cond);
		}
		if (bg->status == (char)'O') {
		}
		pthread_cond_wait(&bg->cond, &bg->mutex);
	}
	pthread_mutex_unlock(&bg->mutex);
	pthread_exit(NULL);
}

int main(void)
{
	shared_memory_t shm;

	int exit_code = 0;

    shm.fd = shm_open(SHARED_NAME, O_RDWR, 438);
    if (shm.fd < 0) {
        shm.data = NULL;
        exit_code = -1;
    }

    shm.data = mmap(NULL, sizeof(shared_data_t), PROT_READ | PROT_WRITE, MAP_SHARED ,shm.fd, 0);
    if (shm.data == MAP_FAILED) {
        exit_code -1;
    }

	if (exit_code != -1) {

		volatile int alarm_active = 0;
		
		pthread_t threads[LEVELS];
		struct monitor_args args[LEVELS];

		for (int i = 0; i < LEVELS; i++) {
			args[i].level = i;
			args[i].alarm_active = &alarm_active;
			args[i].data = shm.data;
			pthread_create(&threads[i], NULL, tempmonitor, (void *)&args[i]);
		}
		for (;;) {
			if (alarm_active == 1) {
				// Handle the alarm system and open boom gates
				// Activate alarms on all levels
				for (int i = 0; i < LEVELS; i++) {
					shm.data->level_collection[i].alarm = 1;
				}
				
				// Open up all boom gates
				pthread_t boomgatethreads[EXITS + ENTRANCES];
				for (int i = 0; i < ENTRANCES; i++) {
					pthread_create(&boomgatethreads[i], NULL, openboomgate, (void*)&shm.data->entrance_collection[i].gate);
				}
				for (int i = 0; i < EXITS; i++) {
					pthread_create(&boomgatethreads[i + ENTRANCES], NULL, openboomgate, (void*)&shm.data->exit_collection[i].gate);
				}
				
				// Show evacuation message on an endless loop
				for (;;) {
					const char *evacmessage = "EVACUATE ";
					for (const char *p = evacmessage; *p != '\0'; p++) {
						for (int i = 0; i < ENTRANCES; i++) {
							;
							pthread_mutex_lock(&shm.data->entrance_collection[i].sign.mutex);
							shm.data->entrance_collection[i].sign.display = *p;
							pthread_cond_broadcast(&shm.data->entrance_collection[i].sign.cond);
							pthread_mutex_unlock(&shm.data->entrance_collection[i].sign.mutex);
						}
						ms_sleep(20);
					}
				}
			}
			ms_sleep(1);
		}
	}
	return exit_code;
}
