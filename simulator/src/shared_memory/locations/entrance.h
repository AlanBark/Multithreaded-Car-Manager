#pragma once
#include <pthread.h>
#include "gate.h"
#include "sign.h"
#include "lpr.h"

typedef struct entrance {

    lpr_t lpr;

    gate_t gate;

    sign_t sign;

} entrance_t;

void initialize_entrance(entrance_t *entrance);