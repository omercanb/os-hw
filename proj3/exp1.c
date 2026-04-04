#include "rsm.h"
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_PROCS 10
#define ROUNDS 3

// Workers grab resources in opposite orders based on apid to create circular wait potential
void worker(int apid, int numr, int avoid) {
    rsm_process_started(apid);
    if (avoid) {
        int claim[MAX_RT];
        for (int i = 0; i < numr; i++)
            claim[i] = 2;
        rsm_claim(claim);
    }

    for (int r = 0; r < ROUNDS; r++) {
        int req1[MAX_RT], req2[MAX_RT];
        for (int i = 0; i < numr; i++) {
            req1[i] = 0;
            req2[i] = 0;
        }

        if (apid % 2 == 0) {
            // Even: grab low resources first, then high
            for (int i = 0; i < numr / 2; i++)
                req1[i] = 1;
            for (int i = numr / 2; i < numr; i++)
                req2[i] = 1;
        } else {
            // Odd: grab high resources first, then low
            for (int i = numr / 2; i < numr; i++)
                req1[i] = 1;
            for (int i = 0; i < numr / 2; i++)
                req2[i] = 1;
        }

        rsm_request(req1);
        usleep(80000); // hold 80ms while requesting more
        rsm_request(req2);
        usleep(50000); // hold both for 50ms
        rsm_release(req2);
        rsm_release(req1);
    }

    rsm_process_ended();
    exit(0);
}

double run_trial(int nump, int numr, int *exist, int avoid, int timeout_ms, int *deadlocked) {
    rsm_init(nump, numr, exist, avoid);

    struct timeval start, end;
    gettimeofday(&start, NULL);

    pid_t pids[MAX_PROCS];
    for (int i = 0; i < nump; i++) {
        pids[i] = fork();
        if (pids[i] == 0)
            worker(i, numr, avoid);
    }

    *deadlocked = 0;
    int finished = 0;
    int checks = timeout_ms / 50;
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

typedef struct {
    int nump;
    int numr;
    int instances;
} config_t;

int main() {
    int trials = 2;

    config_t configs[] = {
        {3, 4, 2},
        {4, 6, 3},
        {6, 6, 4},
        {6, 8, 4},
        {8, 8, 5},
        {10, 10, 6},
    };
    int num_configs = sizeof(configs) / sizeof(configs[0]);

    printf("=== Experiment 1: Avoidance Overhead vs Deadlock Cost ===\n");
    printf("Rounds per process: %d, Trials: %d\n\n", ROUNDS, trials);

    printf("%-8s %-8s %-10s | %-12s %-10s %-10s | %-12s\n",
           "Procs", "RTypes", "Instances",
           "NoAvoid(s)", "Deadlocks", "Completed",
           "Avoid(s)");
    printf("----------------------------------------------------------------------\n");

    for (int c = 0; c < num_configs; c++) {
        int nump = configs[c].nump;
        int numr = configs[c].numr;
        int inst = configs[c].instances;

        int exist[MAX_RT];
        for (int i = 0; i < numr; i++)
            exist[i] = inst;

        double total_no = 0;
        int deadlocks_no = 0;
        int completed_no = 0;
        for (int t = 0; t < trials; t++) {
            int dl = 0;
            double elapsed = run_trial(nump, numr, exist, 0, 15000, &dl);
            total_no += elapsed;
            if (dl > 0)
                deadlocks_no++;
            else
                completed_no++;
        }

        double total_av = 0;
        for (int t = 0; t < trials; t++) {
            int dl = 0;
            double elapsed = run_trial(nump, numr, exist, 1, 30000, &dl);
            total_av += elapsed;
        }

        printf("%-8d %-8d %-10d | %-12.4f %-10d %-10d | %-12.4f\n",
               nump, numr, inst,
               total_no / trials, deadlocks_no, completed_no,
               total_av / trials);
    }

    return 0;
}
