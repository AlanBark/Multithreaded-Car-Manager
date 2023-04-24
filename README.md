# Multi-threaded car park simulator in C

This program simulates a car park with multiple entry and exit points, with the simulator and manager programs managing shared memory. The simulator program generates cars and the manager program manages the car park. The manager program also manages a fire alarm system. 

The program is written to comply with MISRA C to the best of my ability. A noteable exception is the sleep function in the fire alarm. On an embedded system you would want to use the embedded systems clock to sleep, but this is not possible on a desktop system.

## build

`make` generates 3 files, fire_alarm.out, manager.out and simulator.out

`make clean` to clean all build directories and executables.

## run

The simulator should be started first, then the manager, then the fire alarm.