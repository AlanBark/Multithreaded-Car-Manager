#include "exit.h"

void initialize_exit(exit_t *exit) {
    initialize_lpr(&exit->lpr);
    initialize_gate(&exit->gate);
}