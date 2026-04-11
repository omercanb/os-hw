#include "rsm.h"
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

#define NUMR 3
#define ROUNDS 3
#define MAX_PROCS 12

int exist[NUMR] = {4, 4, 4};

void worker(int apid, int nump, int avoid) {
    rsm_process_started(apid);
    if (avoid) {
        int claim[NUMR] = {2, 2, 2};
        rsm_claim(claim);
    }

    for (int r = 0; r < ROUNDS; r++) {
        if (apid % 2 == 0) {
            rsm_request((int[]){1, 0, 1});
            usleep(80000);
            rsm_request((int[]){0, 1, 0});
            usleep(40000);
            rsm_release((int[]){0, 1, 0});
            rsm_release((int[]){1, 0, 1});
        } else {
            rsm_request((int[]){0, 1, 0});
            usleep(80000);
            rsm_request((int[]){1, 0, 1});
            usleep(40000);
            rsm_release((int[]){1, 0, 1});
            rsm_release((int[]){0, 1, 0});
        }
    }

    rsm_process_ended();
    exit(0);
}

double run_trial(int nump, int avoid, int *deadlocked) {
    rsm_init(nump, NUMR, exist, avoid);

    struct timeval start, end;
    gettimeofday(&start, NULL);

    pid_t pids[MAX_PROCS];
    for (int i = 0; i < nump; i++) {
        pids[i] = fork();
        if (pids[i] == 0)
            worker(i, nump, avoid);
    }

    *deadlocked = 0;
    int finished = 0;
    int checks = 20000 / 50; // 20 sec timeout
    for (int t = 0; t < checks && finished < nump; t++) {
        usleep(50000);
        while (waitpid(-1, NULL, WNOHANG) > 0)
            finished++;
        if (!avoid && finished < nump) {
            int dl = rsm_detection();
            if (dl > 0) {
                *deadlocked = dl;
                for (int i = 0; i < nump; i++)
                    kill(pids[i], SIGKILL);
                for (int i = 0; i < nump; i++)
                    waitpid(pids[i], NULL, 0);
                gettimeofday(&end, NULL);
                rsm_destroy();
                return (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
            }
        }
    }

    for (int i = 0; i < nump; i++)
        kill(pids[i], SIGKILL);
    for (int i = 0; i < nump; i++)
        waitpid(pids[i], NULL, 0);

    gettimeofday(&end, NULL);
    rsm_destroy();
    return (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
}

int main() {
    int trials = 5;
    int proc_counts[] = {2, 3, 4, 6, 8, 10, 12};
    int num_counts = sizeof(proc_counts) / sizeof(proc_counts[0]);

    printf("=== Experiment 3: Scaling Number of Processes ===\n");
    printf("Resource types: %d, Instances per type: %d, Rounds: %d\n\n",
           NUMR, exist[0], ROUNDS);

    for (int avoid = 0; avoid <= 1; avoid++) {
        printf("--- %s ---\n", avoid ? "With Avoidance" : "No Avoidance");
        printf("%-12s %-12s %-12s %-12s\n", "Processes", "Avg Time(s)", "Deadlocks", "Completed");

        for (int p = 0; p < num_counts; p++) {
            double total_time = 0;
            int total_deadlocks = 0;
            int completed = 0;

            for (int t = 0; t < trials; t++) {
                int dl = 0;
                double elapsed = run_trial(proc_counts[p], avoid, &dl);
                total_time += elapsed;
                if (dl > 0)
                    total_deadlocks++;
                else
                    completed++;
            }

            printf("%-12d %-12.4f %-12d %-12d\n",
                   proc_counts[p], total_time / trials, total_deadlocks, completed);
        }
        printf("\n");
    }

    return 0;
}
