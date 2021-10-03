#include "entrance.h"

void initialize_entrance(entrance_t *entrance) {
    initialize_lpr(&entrance->lpr);
    initialize_gate(&entrance->gate);
    initialize_sign(&entrance->sign);
}