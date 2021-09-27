#include <stdbool.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include "lpr.h"

int main(int argc, char **argv) {
    lpr_t * lpr = (lpr_t *)malloc(sizeof(lpr_t));
    updatePlate(lpr, "abc123");
    printf("%s\n", lpr->licensePlate);
    free(lpr);
    return 0;
}

