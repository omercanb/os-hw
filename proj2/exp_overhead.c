#include "tus.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define TOTAL_YIELDS 100000 // total yields shared across all threads
#define MAX_TEST 63         // max threads we can create

volatile int sink;
void busy_work(int n) {
    for (int i = 0; i < n; i++)
        sink = i;
}

typedef struct {
    int num_yields;
    int work_per_yield;
} worker_arg;

void *worker(void *arg) {
    worker_arg *w = (worker_arg *)arg;
    for (int i = 0; i < w->num_yields; i++) {
        busy_work(w->work_per_yield);
        tus_yield(TUS_ANY);
    }
    return NULL;
}

double run_trial(int num_threads, int alg) {
    tus_init(alg);

    int yields_per_thread = TOTAL_YIELDS / num_threads;
    int work_per_yield = 100; // small, so timing is dominated by yield/queue

    worker_arg *args = malloc(num_threads * sizeof(worker_arg));
    int *tids = malloc(num_threads * sizeof(int));

    for (int i = 0; i < num_threads; i++) {
        args[i].num_yields = yields_per_thread;
        args[i].work_per_yield = work_per_yield;
        tids[i] = tus_create_thread(worker, &args[i]);
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    tus_yield(TUS_ANY);

    for (int i = 0; i < num_threads; i++) {
        tus_join(tids[i]);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    double sec = end.tv_sec - start.tv_sec;
    double nsec = end.tv_nsec - start.tv_nsec;
    double total_ms = sec * 1000.0 + nsec / 1000000.0;

    free(args);
    free(tids);

    return total_ms;
}

int main(int argc, char **argv) {
    int thread_counts[] = {1, 2, 4, 8, 16, 32, 63};
    int num_tests = sizeof(thread_counts) / sizeof(thread_counts[0]);

    printf("=== EXPERIMENT: Scaling with number of threads ===\n");
    printf("total yields fixed at %d, split evenly across threads\n\n", TOTAL_YIELDS);

    printf("%8s  %12s  %12s  %16s\n",
           "threads", "yields/thr", "total (ms)", "us/yield");
    printf("----------------------------------------------------------\n");

    for (int t = 0; t < num_tests; t++) {
        int n = thread_counts[t];
        int yields_per = TOTAL_YIELDS / n;

        // run 3 trials, take average
        double total = 0;
        int trials = 3;
        for (int r = 0; r < trials; r++) {
            total += run_trial(n, ALG_FCFS);
        }
        double avg_ms = total / trials;
        double us_per_yield = (avg_ms * 1000.0) / TOTAL_YIELDS;

        printf("%8d  %12d  %12.2f  %16.4f\n",
               n, yields_per, avg_ms, us_per_yield);
    }

    printf("\n");
    // final tus_exit to terminate
    // (last run_trial already exited, so we just exit normally)
    return 0;
}
