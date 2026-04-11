#include "rsm.h"
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

#define NUMP 4
#define NUMR 3
#define ROUNDS 3

void worker(int apid, int avoidance, int resource_level) {
    rsm_process_started(apid);
    if (avoidance) {
        int claim[NUMR] = {2, 2, 2};
        rsm_claim(claim);
    }

    for (int r = 0; r < ROUNDS; r++) {
        if (apid % 2 == 0) {
            rsm_request((int[]){1, 0, 1});
            usleep(100000);
            rsm_request((int[]){0, 1, 0});
            usleep(50000);
            rsm_release((int[]){0, 1, 0});
            rsm_release((int[]){1, 0, 1});
        } else {
            rsm_request((int[]){0, 1, 0});
            usleep(100000);
            rsm_request((int[]){1, 0, 1});
            usleep(50000);
            rsm_release((int[]){1, 0, 1});
            rsm_release((int[]){0, 1, 0});
        }
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

    pid_t pids[NUMP];
    for (int i = 0; i < NUMP; i++) {
        pids[i] = fork();
        if (pids[i] == 0)
            worker(i, avoid, resource_level);
    }

    *deadlocked = 0;
    int finished = 0;
    int checks = 15000 / 50; // 15 sec timeout, 50ms intervals
    for (int t = 0; t < checks && finished < NUMP; t++) {
        usleep(50000);
        while (waitpid(-1, NULL, WNOHANG) > 0)
            finished++;
        if (!avoid && finished < NUMP) {
            int dl = rsm_detection();
            if (dl > 0) {
                *deadlocked = dl;
                for (int i = 0; i < NUMP; i++)
                    kill(pids[i], SIGKILL);
                for (int i = 0; i < NUMP; i++)
                    waitpid(pids[i], NULL, 0);
                gettimeofday(&end, NULL);
                rsm_destroy();
                return (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
            }
        }
    }

    for (int i = 0; i < NUMP; i++)
        kill(pids[i], SIGKILL);
    for (int i = 0; i < NUMP; i++)
        waitpid(pids[i], NULL, 0);

    gettimeofday(&end, NULL);
    rsm_destroy();
    return (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
}

int main() {
    int trials = 5;
    int levels[] = {2, 3, 4, 6, 10};
    int num_levels = sizeof(levels) / sizeof(levels[0]);

    printf("=== Experiment 2: Resource Scarcity vs Throughput ===\n");
    printf("Processes: %d, Resource types: %d, Rounds: %d\n\n", NUMP, NUMR, ROUNDS);

    for (int avoid = 0; avoid <= 1; avoid++) {
        printf("--- %s ---\n", avoid ? "With Avoidance" : "No Avoidance");
        printf("%-12s %-12s %-12s %-12s\n", "Instances", "Avg Time(s)", "Deadlocks", "Completed");

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

            printf("%-12d %-12.4f %-12d %-12d\n",
                   levels[l], total_time / trials, total_deadlocks, completed);
        }
        printf("\n");
    }

    return 0;
}
