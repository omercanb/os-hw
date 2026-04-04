#ifndef RSM_H
#define RSM_H

#include <pthread.h>
#include <stdbool.h>

#define MAX_RT 100 // max num of resource types supported
#define MAX_PR 100 // max num of processes supported

typedef struct {
    // Total resources existing for each resource type
    int total_resources[MAX_RT];
    // Resources currently available for each resource type
    int available[MAX_RT];
    // Resources allocated to each process for each resource type
    int allocated[MAX_PR][MAX_RT];
    // Queued requests of each process for each resource type
    int requested[MAX_PR][MAX_RT];

    // avoidance
    // Declared max resource usages
    int max[MAX_PR][MAX_RT];
    // We calculate need from max - allocated
    // A map of app_ids to pids (needs linear search)
    int pids[MAX_PR];
    // A count of initizalied processes so to wait for all processes to initialize
    // before allocating resources.
    // I don't think this is required for correctness but it's in the spec
    int num_procs_initialized;
    pthread_cond_t all_procs_initialized_cond;
    // Condition variables for each resource
    pthread_cond_t resource_cond;
    pthread_mutex_t mutex;
    int test;
} state_t;

enum { SAFE,
       UNSAFE };

int rsm_init(int p_count, int r_count,
             int exist[], int avoid);
int rsm_destroy();
int rsm_process_started(int apid);
int rsm_process_ended();
int rsm_claim(int claim[]); // only for avoidance
int rsm_request(int request[]);
int rsm_release(int release[]);
int rsm_detection();
void rsm_print_state(char headermsg[]);
void _rsm_set_avoidance(bool avoidance);

int bankers(int request[]);

#endif /* RSM_H */
