#include "rsm.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

#define NUMP 4
#define NUMR 3
#define ROUNDS 4

void worker(int apid, int avoidance, int resource_level) {
    rsm_process_started(apid);
    if (avoidance) {
        int claim[NUMR] = {2, 2, 2};
        rsm_claim(claim);
    }

    for (int r = 0; r < ROUNDS; r++) {
        int req[NUMR] = {1, 1, 1};
        rsm_request(req);
        usleep(40000);
        rsm_release(req);

        int req2[NUMR] = {1, 1, 0};
        rsm_request(req2);
        usleep(20000);
        rsm_release(req2);
    }

    rsm_process_ended();
    exit(0);
}

double run_trial(int resource_level, int avoid, int *deadlocked) {
    int exist[NUMR];
    for (int i = 0; i < NUMR; i++)
        exist[i] = resource_level;

    rsm_init(NUMP, NUMR, exist, avoid);

    struct timeval start, end;
    gettimeofday(&start, NULL);

    for (int i = 0; i < NUMP; i++) {
        if (fork() == 0)
            worker(i, avoid, resource_level);
    }

    *deadlocked = 0;
    int finished = 0;
    for (int t = 0; t < 15 && finished < NUMP; t++) {
        sleep(1);
        while (waitpid(-1, NULL, WNOHANG) > 0)
            finished++;
        if (!avoid) {
            int dl = rsm_detection();
            if (dl > 0) {
                *deadlocked = dl;
                for (int i = 0; i < NUMP; i++)
                    wait(NULL);
                gettimeofday(&end, NULL);
                rsm_destroy();
                return (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
            }
        }
    }

    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;

    gettimeofday(&end, NULL);
    rsm_destroy();
    return (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
}

int main() {
    int trials = 5;
    int levels[] = {2, 3, 4, 6, 8, 12};
    int num_levels = sizeof(levels) / sizeof(levels[0]);

    printf("=== Experiment 2: Resource Scarcity vs Throughput ===\n");
    printf("Processes: %d, Resource types: %d, Rounds: %d\n\n", NUMP, NUMR, ROUNDS);

    for (int avoid = 0; avoid <= 1; avoid++) {
        printf("--- %s ---\n", avoid ? "With Avoidance" : "No Avoidance");
        printf("%-12s %-12s %-12s %-12s\n", "Instances", "Avg Time", "Deadlocks", "Completed");

        for (int l = 0; l < num_levels; l++) {
            double total_time = 0;
            int total_deadlocks = 0;
            int completed = 0;

            for (int t = 0; t < trials; t++) {
                int dl = 0;
                double elapsed = run_trial(levels[l], avoid, &dl);
                total_time += elapsed;
                if (dl > 0)
                    total_deadlocks++;
                else
                    completed++;
            }

            printf("%-12d %-12.3f %-12d %-12d\n",
                   levels[l], total_time / trials, total_deadlocks, completed);
        }
        printf("\n");
    }

    return 0;
}
