#include "tus.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define ITERATIONS 100000

static int tid_a, tid_b;
static int count_a = 0, count_b = 0;

void *ping(void *arg) {
    while (count_a < ITERATIONS) {
        count_a++;
        tus_yield(tid_b);
    }
    return NULL;
}

void *pong(void *arg) {
    while (count_b < ITERATIONS) {
        count_b++;
        tus_yield(tid_a);
    }
    return NULL;
}

int main() {
    tus_init(ALG_FCFS);

    tid_a = tus_create_thread(ping, NULL);
    tid_b = tus_create_thread(pong, NULL);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    tus_yield(tid_a);

    clock_gettime(CLOCK_MONOTONIC, &end);

    double sec = end.tv_sec - start.tv_sec;
    double nsec = end.tv_nsec - start.tv_nsec;
    double total_ms = sec * 1000.0 + nsec / 1000000.0;

    int total_switches = count_a + count_b;

    printf("=== EXPERIMENT: Context switch cost ===\n");
    printf("total switches:  %d\n", total_switches);
    printf("total time:      %.2f ms\n", total_ms);
    printf("avg per switch:  %.4f us\n", (total_ms * 1000.0) / total_switches);

    tus_exit();
    return 0;
}
