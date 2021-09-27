#include "lpr.h"
#include <string.h>

void updatePlate(lpr_t* lpr, char plate[6]) {
    memcpy(lpr->licensePlate, plate, 6);
}