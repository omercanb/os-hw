#include "rsm.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

#define NUMP 5
#define NUMR 5
#define ROUNDS 3

int exist[NUMR] = {4, 4, 3, 3, 3};

void worker(int apid) {
    rsm_process_started(apid);
    int claim[NUMR] = {2, 2, 2, 2, 2};
    rsm_claim(claim);

    for (int r = 0; r < ROUNDS; r++) {
        int req[NUMR] = {1, 1, 1, 1, 1};
        rsm_request(req);
        usleep(50000);
        rsm_release(req);

        int req2[NUMR] = {1, 0, 1, 0, 1};
        rsm_request(req2);
        usleep(30000);
        rsm_release(req2);
    }

    rsm_process_ended();
    exit(0);
}

double run_trial(int avoid, int timeout_sec) {
    rsm_init(NUMP, NUMR, exist, avoid);

    struct timeval start, end;
    gettimeofday(&start, NULL);

    for (int i = 0; i < NUMP; i++) {
        if (fork() == 0)
            worker(i);
    }

    // Wait with timeout for deadlock case
    int finished = 0;
    for (int t = 0; t < timeout_sec && finished < NUMP; t++) {
        sleep(1);
        int status;
        while (waitpid(-1, &status, WNOHANG) > 0)
            finished++;
        if (!avoid) {
            int dl = rsm_detection();
            if (dl > 0) {
                printf("  deadlock detected: %d processes\n", dl);
                // Kill remaining children
                for (int i = 0; i < NUMP; i++)
                    wait(NULL);
                gettimeofday(&end, NULL);
                rsm_destroy();
                return -1.0; // deadlock marker
            }
        }
    }

    // Collect any remaining
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;

    gettimeofday(&end, NULL);
    rsm_destroy();
    return (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
}

int main() {
    int trials = 5;
    printf("=== Experiment 1: Avoidance Overhead vs Deadlock Cost ===\n");
    printf("Processes: %d, Resources: %d, Rounds per process: %d\n\n", NUMP, NUMR, ROUNDS);

    printf("--- No Avoidance (flag=0) ---\n");
    int deadlocks = 0;
    double total_time_no = 0;
    for (int t = 0; t < trials; t++) {
        double elapsed = run_trial(0, 10);
        if (elapsed < 0) {
            deadlocks++;
            printf("  Trial %d: DEADLOCK\n", t + 1);
        } else {
            total_time_no += elapsed;
            printf("  Trial %d: %.3f sec\n", t + 1, elapsed);
        }
    }
    printf("Deadlocks: %d/%d trials\n", deadlocks, trials);
    if (deadlocks < trials)
        printf("Avg completion (non-deadlock): %.3f sec\n", total_time_no / (trials - deadlocks));

    printf("\n--- With Avoidance (flag=1) ---\n");
    double total_time_av = 0;
    for (int t = 0; t < trials; t++) {
        double elapsed = run_trial(1, 30);
        total_time_av += elapsed;
        printf("  Trial %d: %.3f sec\n", t + 1, elapsed);
    }
    printf("Avg completion: %.3f sec\n", total_time_av / trials);

    return 0;
}
