#include "tus.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM_THREADS 10
#define BASE_WORK 50000000

static struct timespec start_time;
static double finish_times[NUM_THREADS + 10]; // indexed by tid

double elapsed_ms() {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    double sec = now.tv_sec - start_time.tv_sec;
    double nsec = now.tv_nsec - start_time.tv_nsec;
    return sec * 1000.0 + nsec / 1000000.0;
}

volatile int sink;
void busy_work(int iterations) {
    for (int i = 0; i < iterations; i++) {
        sink = i;
    }
}

void *worker(void *arg) {
    int rank = *(int *)arg; // 1 = longest, NUM_THREADS = shortest
    int mytid = tus_gettid();
    int total_work = BASE_WORK * (NUM_THREADS - rank + 1);

    busy_work(total_work);

    finish_times[mytid] = elapsed_ms();
    printf("thread %2d (rank %2d, work=%8d) finished at %8.2f ms\n",
           mytid, rank, total_work, finish_times[mytid]);
    return NULL;
}

void run_experiment(int alg, const char *alg_name) {
    tus_init(alg);
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    int ranks[NUM_THREADS];
    int tids[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++) {
        ranks[i] = i + 1;
        tids[i] = tus_create_thread(worker, &ranks[i]);
    }

    // each thread runs to completion, then tus_exit picks next
    for (int i = 0; i < NUM_THREADS; i++) {
        tus_join(tids[i]);
    }

    printf("\n--- %s summary ---\n", alg_name);
    printf("%6s  %10s  %12s\n", "rank", "work", "finish (ms)");
    for (int i = 0; i < NUM_THREADS; i++) {
        int rank = i + 1;
        int work = BASE_WORK * (NUM_THREADS - rank + 1);
        printf("%6d  %10d  %12.2f\n", rank, work, finish_times[tids[i]]);
    }

    printf("\n");
    tus_exit();
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("usage: %s <fcfs|random>\n", argv[0]);
        return 1;
    }

    if (argv[1][0] == 'f') {
        printf("=== EXPERIMENT: FCFS scheduling ===\n\n");
        run_experiment(ALG_FCFS, "FCFS");
    } else {
        printf("=== EXPERIMENT: RANDOM scheduling ===\n\n");
        run_experiment(ALG_RANDOM, "RANDOM");
    }

    return 0;
}
