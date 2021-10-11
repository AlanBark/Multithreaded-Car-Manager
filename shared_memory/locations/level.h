#pragma once
#include <pthread.h>
#include <stdint.h>
#include "lpr.h"

typedef struct level {

    lpr_t lpr;

    volatile int16_t sensor;

    volatile int alarm;

} level_t;

void initialize_level(level_t *level);