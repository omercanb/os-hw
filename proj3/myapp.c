#include "rsm.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#define NUMP 4
#define NUMR 5

int AVOID = 0;
int exist[NUMR] = {3, 3, 2, 2, 2};

// Designed to create circular wait when avoidance is off:
//   P0 grabs R0,R1 then wants R2,R3
//   P1 grabs R2,R3 then wants R0,R1
//   P2 grabs R4 then wants R2
//   P3 wants R4

void func_p0(int apid) {
    rsm_process_started(apid);
    rsm_claim((int[]){2, 2, 2, 2, 0});

    rsm_request((int[]){2, 2, 0, 0, 0});
    sleep(2);
    rsm_request((int[]){0, 0, 2, 2, 0});

    sleep(1);
    rsm_release((int[]){2, 2, 0, 0, 0});
    rsm_release((int[]){0, 0, 2, 2, 0});
    rsm_process_ended();
    exit(0);
}

void func_p1(int apid) {
    rsm_process_started(apid);
    rsm_claim((int[]){2, 2, 2, 2, 0});

    rsm_request((int[]){0, 0, 2, 2, 0});
    sleep(2);
    rsm_request((int[]){2, 2, 0, 0, 0});

    sleep(1);
    rsm_release((int[]){0, 0, 2, 2, 0});
    rsm_release((int[]){2, 2, 0, 0, 0});
    rsm_process_ended();
    exit(0);
}

void func_p2(int apid) {
    rsm_process_started(apid);
    rsm_claim((int[]){0, 0, 1, 0, 2});

    rsm_request((int[]){0, 0, 0, 0, 2});
    sleep(2);
    rsm_request((int[]){0, 0, 1, 0, 0});

    sleep(1);
    rsm_release((int[]){0, 0, 0, 0, 2});
    rsm_release((int[]){0, 0, 1, 0, 0});
    rsm_process_ended();
    exit(0);
}

void func_p3(int apid) {
    rsm_process_started(apid);
    rsm_claim((int[]){0, 0, 0, 0, 2});

    sleep(1);
    rsm_request((int[]){0, 0, 0, 0, 2});

    sleep(1);
    rsm_release((int[]){0, 0, 0, 0, 2});
    rsm_process_ended();
    exit(0);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("usage: ./myapp <0|1>\n");
        exit(1);
    }

    AVOID = atoi(argv[1]);
    rsm_init(NUMP, NUMR, exist, AVOID);

    void (*funcs[])(int) = {func_p0, func_p1, func_p2, func_p3};

    for (int i = 0; i < NUMP; i++) {
        if (fork() == 0) {
            funcs[i](i);
        }
    }

    // Monitor loop
    int deadlock_detected = 0;
    for (int t = 0; t < 15; t++) {
        sleep(1);
        rsm_print_state("State");
        int ret = rsm_detection();
        if (ret > 0 && !deadlock_detected) {
            deadlock_detected = 1;
            printf("deadlock: %d processes\n", ret);
            rsm_print_state("Deadlock state");
            break;
        }
    }

    for (int i = 0; i < NUMP; i++) {
        wait(NULL);
    }

    rsm_destroy();
    return 0;
}
