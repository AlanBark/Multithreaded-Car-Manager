#pragma once
#include <pthread.h>
#include "gate.h"
#include "lpr.h"

typedef struct exit {

    lpr_t lpr;

    gate_t gate;

} exit_t;

void initialize_exit(exit_t *exit);