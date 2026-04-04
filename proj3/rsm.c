#include "rsm.h"
#include "assert.h"
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>

#define TRUE 1
#define FALSE 0

int num_procs;     // number of processes
int num_resources; // number of resource types
int fd;
state_t *state;
bool avoidance;

void _rsm_set_avoidance(bool avoidance) {
    avoidance = avoidance;
}

int get_proc() {
    int pid = getpid();
    for (int proc = 0; proc < num_procs; proc++) {
        if (state->pids[proc] == pid) {
            return proc;
        }
    }
    assert(false);
}

int bankers(int request[]) {
    int calling_proc = get_proc();
    bool finished[num_procs];
    for (int proc = 0; proc < num_procs; proc++) {
        finished[proc] = false;
    }
    int available[num_resources];
    for (int resource = 0; resource < num_resources; resource++) {
        available[resource] = state->available[resource] - request[resource];
    }
    // Find processes that can finish (have need <= available)
    // Add their allocation to availbale
    // If you can't find one, not safe
    int completion_order[num_procs];
    int idx = 0;

    int num_procs_finished = 0;
    while (num_procs_finished < num_procs) {
        bool can_allocate = false;
        // Do a pass over all processes to see if you can select a process
        // (This algorithm can select multiple processes in a single pass)
        for (int proc = 0; proc < num_procs; proc++) {
            if (finished[proc]) {
                continue;
            }
            // Test if need <= available for all resources of a process
            bool allocation_need_met = true;
            for (int resource = 0; resource < num_resources; resource++) {
                int need;
                if (proc == calling_proc) {
                    // Resource needs to be a part of allocation for the resoruce requesting process
                    // Instead of holding a copy we do this computation
                    need = state->max[proc][resource] - (state->allocated[proc][resource] + request[resource]);
                } else {
                    need = state->max[proc][resource] - state->allocated[proc][resource];
                }
                if (available[resource] < need) {
                    allocation_need_met = false;
                    break;
                }
            }
            // Simulate the selected process finishing and leaving resources
            if (allocation_need_met) {
                num_procs_finished++;
                can_allocate = true;
                completion_order[idx++] = proc;
                finished[proc] = true;
                for (int resource = 0; resource < num_resources; resource++) {
                    if (proc == calling_proc) {
                        available[resource] += (state->allocated[proc][resource] + request[resource]);
                    } else {
                        available[resource] += state->allocated[proc][resource];
                    }
                }
            }
        }
        if (!can_allocate) {
            return UNSAFE;
        }
    }
    return SAFE;
}

bool resources_available(int request[]) {
    for (int resource = 0; resource < num_resources; resource++) {
        if (state->available[resource] < request[resource]) {
            return false;
        }
    }
    return true;
}

// Initialize shared memory
int rsm_init(int p_count, int r_count, int exist[], int avoid) {
    avoidance = avoid;
    // Create the shared memory setup
    fd = shm_open("oshw", O_CREAT | O_RDWR, 0666);
    ftruncate(fd, sizeof(state_t));
    state = mmap(0, sizeof(state_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    memset(state, 0x00, sizeof(state_t));

    // Assign state
    num_procs = p_count;
    num_resources = r_count;
    state->num_procs_initialized = 0;
    for (int i = 0; i < num_resources; i++) {
        state->total_resources[i] = exist[i];
        state->available[i] = exist[i];
    }

    // Initialize the condition variable and mutex for multiple processes
    pthread_condattr_t cattr;
    pthread_condattr_init(&cattr);
    pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&state->resource_cond, &cattr);
    pthread_condattr_destroy(&cattr);

    pthread_condattr_init(&cattr);
    pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&state->all_procs_initialized_cond, &cattr);
    pthread_condattr_destroy(&cattr);

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    // Recursive mutex allows mutexes to be shared in a thread
    // It's used so print state (which is called by other rsm functions when debugging) doesnt overlap with other prints
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&state->mutex, &attr);
    pthread_mutexattr_destroy(&attr);

    // rsm_print_state("Initial");

    return 0;
}

int rsm_destroy() {
    munmap(state, sizeof(state_t));
    close(fd);
    shm_unlink("oshw");
    return 0;
}

// We need to maintain a map of pids to indices into pids
int rsm_process_started(int app_id) {
    pthread_mutex_lock(&state->mutex);
    assert(0 <= app_id && app_id < num_procs);
    state->pids[app_id] = getpid();
    state->num_procs_initialized++;
    if (state->num_procs_initialized == num_procs) {
        pthread_cond_broadcast(&state->all_procs_initialized_cond);
    }
    pthread_mutex_unlock(&state->mutex);
    return 0;
}

int rsm_process_ended() {
    pthread_mutex_lock(&state->mutex);
    int proc = get_proc();

    for (int resource = 0; resource < num_resources; resource++) {
        state->available[resource] += state->allocated[proc][resource];
        state->allocated[proc][resource] = 0;
        state->max[proc][resource] = 0;
        state->requested[proc][resource] = 0;
    }
    pthread_cond_broadcast(&state->resource_cond);
    pthread_mutex_unlock(&state->mutex);
    return 0;
}

int rsm_claim(int claim[]) {
    pthread_mutex_lock(&state->mutex);
    int proc = get_proc();
    for (int resource = 0; resource < num_resources; resource++) {
        if (state->total_resources[resource] < claim[resource]) {
            pthread_mutex_unlock(&state->mutex);
            return -1;
        } else {
            state->max[proc][resource] = claim[resource];
        }
    }
    pthread_mutex_unlock(&state->mutex);
    return 0;
}

int rsm_request(int request[]) {
    pthread_mutex_lock(&state->mutex);
    // An if statement also works for this because num proc initialized only increases
    while (state->num_procs_initialized < num_procs) {
        pthread_cond_wait(&state->all_procs_initialized_cond, &state->mutex);
    }

    int proc = get_proc();
    // Check if caller has made any errors
    for (int resource = 0; resource < num_resources; resource++) {
        if (state->total_resources[resource] < request[resource]) {
            pthread_mutex_unlock(&state->mutex);
            return -1;
        }
        if (avoidance) {
            // The request cannot request more than it's declared max
            if (state->allocated[proc][resource] + request[resource] > state->max[proc][resource]) {
                assert(false);
            }
        }
    }

    // Update the requested array to view when it's in a waiting state
    for (int resource = 0; resource < num_resources; resource++) {
        state->requested[proc][resource] = request[resource];
    }

    if (avoidance) {
        // Use the bankers algorithm to check if the request leads to an unsafe state
        // Assume the request is accepted then test if the state is safe
        while (bankers(request) == UNSAFE) {
            // printf("For proc: %d and for request: [", proc);
            // for (int resource = 0; resource < num_resources; resource++) {
            //     printf("%d ", request[resource]);
            // }
            // printf("]\n");

            // rsm_print_state("Unsafe state reached");
            pthread_cond_wait(&state->resource_cond, &state->mutex);
        }
    } else {
        // Directly check if the current resources are enough
        while (!resources_available(request)) {
            // printf("For proc: %d and for request: [", proc);
            // for (int resource = 0; resource < num_resources; resource++) {
            //     printf("%d ", request[resource]);
            // }
            // printf("]\n");

            // rsm_print_state("Can't complete request (no avoidance)");
            pthread_cond_wait(&state->resource_cond, &state->mutex);
        }
    }

    // Current resources are enough for either avoidance or just resource allocation
    for (int resource = 0; resource < num_resources; resource++) {
        state->allocated[proc][resource] += request[resource];
        state->available[resource] -= request[resource];
    }

    // Zero the requested array
    for (int resource = 0; resource < num_resources; resource++) {
        state->requested[proc][resource] = 0;
    }
    pthread_mutex_unlock(&state->mutex);
    return 0;
}

int rsm_release(int release[]) {
    pthread_mutex_lock(&state->mutex);
    int proc = get_proc();
    // Check if the release array is valid
    for (int resource = 0; resource < num_resources; resource++) {
        if (state->allocated[proc][resource] < release[resource]) {
            pthread_mutex_unlock(&state->mutex);
            return -1;
        }
    }

    for (int resource = 0; resource < num_resources; resource++) {
        state->allocated[proc][resource] -= release[resource];
        state->available[resource] += release[resource];
    }
    pthread_cond_broadcast(&state->resource_cond);
    pthread_mutex_unlock(&state->mutex);
    return 0;
}

int rsm_detection() {
    pthread_mutex_lock(&state->mutex);
    bool finished[num_procs];
    for (int proc = 0; proc < num_procs; proc++) {
        finished[proc] = false;
    }
    int available[num_resources];
    for (int resource = 0; resource < num_resources; resource++) {
        available[resource] = state->available[resource];
    }

    bool can_allocate = true;
    while (can_allocate) {
        // Loop over all processes to see if any can be allocated to
        can_allocate = false;
        for (int proc = 0; proc < num_procs; proc++) {
            if (finished[proc]) {
                continue;
            }
            // Test if request <= available for all resources of a process
            bool allocation_need_met = true;
            for (int resource = 0; resource < num_resources; resource++) {
                if (available[resource] < state->requested[proc][resource]) {
                    allocation_need_met = false;
                    break;
                }
            }
            // Simulate the selected process finishing and leaving resources
            if (allocation_need_met) {
                can_allocate = true;
                finished[proc] = true;
                for (int resource = 0; resource < num_resources; resource++) {
                    available[resource] += state->allocated[proc][resource];
                }
            }
        }
    }
    int num_deadlocks = 0;
    for (int proc = 0; proc < num_procs; proc++) {
        if (!finished[proc]) {
            num_deadlocks++;
        }
    }
    pthread_mutex_unlock(&state->mutex);
    return num_deadlocks;
}

void rsm_print_state_side_by_side(char hmsg[]) {
    int i, j;
    int col_width = 3;  // width per resource column
    int matrix_gap = 4; // gap between matrices
    // Calculate width of one matrix block: "P0:" = 3 chars + num_resources * col_width
    int label_width = 4; // "P0: " etc
    int block_width = label_width + num_resources * col_width;

    printf("##########################\n");
    printf("%s\n", hmsg);
    printf("##########################\n");

    // Exist and Available side by side
    printf("%-*s", block_width + matrix_gap, "Exist:");
    printf("Available:\n");

    // Resource headers
    printf("    ");
    for (j = 0; j < num_resources; j++)
        printf("%-*s", col_width, "R0" + 0 ? "" : ""); // placeholder
    // Print Exist resource headers
    printf("    ");
    for (j = 0; j < num_resources; j++)
        printf("R%-*d", col_width - 1, j);
    for (int g = 0; g < matrix_gap; g++)
        printf(" ");
    printf("    ");
    for (j = 0; j < num_resources; j++)
        printf("R%-*d", col_width - 1, j);
    printf("\n");

    // Exist and Available values
    printf("    ");
    for (j = 0; j < num_resources; j++)
        printf("%-*d", col_width, state->total_resources[j]);
    for (int g = 0; g < matrix_gap; g++)
        printf(" ");
    printf("    ");
    for (j = 0; j < num_resources; j++)
        printf("%-*d", col_width, state->available[j]);
    printf("\n\n");

    // Matrix names
    const char *names[] = {"Allocation", "Request", "MaxDemand", "Need"};
    for (int m = 0; m < 4; m++) {
        printf("%-*s", block_width, names[m]);
        if (m < 3)
            for (int g = 0; g < matrix_gap; g++)
                printf(" ");
    }
    printf("\n");

    // Resource headers for all four matrices
    for (int m = 0; m < 4; m++) {
        printf("    ");
        for (j = 0; j < num_resources; j++)
            printf("R%-*d", col_width - 1, j);
        if (m < 3)
            for (int g = 0; g < matrix_gap; g++)
                printf(" ");
    }
    printf("\n");

    // Process rows
    for (i = 0; i < num_procs; i++) {
        // Allocation
        printf("P%d: ", i);
        for (j = 0; j < num_resources; j++)
            printf("%-*d", col_width, state->allocated[i][j]);
        for (int g = 0; g < matrix_gap; g++)
            printf(" ");

        // Request
        printf("P%d: ", i);
        for (j = 0; j < num_resources; j++)
            printf("%-*d", col_width, state->requested[i][j]);
        for (int g = 0; g < matrix_gap; g++)
            printf(" ");

        // MaxDemand
        printf("P%d: ", i);
        for (j = 0; j < num_resources; j++)
            printf("%-*d", col_width, state->max[i][j]);
        for (int g = 0; g < matrix_gap; g++)
            printf(" ");

        // Need
        printf("P%d: ", i);
        for (j = 0; j < num_resources; j++)
            printf("%-*d", col_width, state->max[i][j] - state->allocated[i][j]);
        printf("\n");
    }

    printf("##########################\n");
}

void rsm_print_state(char hmsg[]) {
    pthread_mutex_lock(&state->mutex);
    rsm_print_state_side_by_side(hmsg);
    pthread_mutex_unlock(&state->mutex);
    return;
    int i, j;

    printf("##########################\n");
    printf("%s\n", hmsg);
    printf("##########################\n");

    // Exist
    printf("Exist:\n");
    for (j = 0; j < num_resources; j++)
        printf("R%d ", j);
    printf("\n");
    for (j = 0; j < num_resources; j++)
        printf("%d  ", state->total_resources[j]);
    printf("\n\n");

    // Available
    printf("Available:\n");
    for (j = 0; j < num_resources; j++)
        printf("R%d ", j);
    printf("\n");
    for (j = 0; j < num_resources; j++)
        printf("%d  ", state->available[j]);
    printf("\n\n");

    // Allocation
    printf("Allocation:\n");
    for (j = 0; j < num_resources; j++)
        printf("   R%d", j);
    printf("\n");
    for (i = 0; i < num_procs; i++) {
        printf("P%d:", i);
        for (j = 0; j < num_resources; j++)
            printf(" %d ", state->allocated[i][j]);
        printf("\n");
    }
    printf("\n");

    // Request
    printf("Request:\n");
    for (j = 0; j < num_resources; j++)
        printf("   R%d", j);
    printf("\n");
    for (i = 0; i < num_procs; i++) {
        printf("P%d:", i);
        for (j = 0; j < num_resources; j++)
            printf(" %d ", state->requested[i][j]);
        printf("\n");
    }
    printf("\n");

    // MaxDemand
    printf("MaxDemand:\n");
    for (j = 0; j < num_resources; j++)
        printf("   R%d", j);
    printf("\n");
    for (i = 0; i < num_procs; i++) {
        printf("P%d:", i);
        for (j = 0; j < num_resources; j++)
            printf(" %d ", state->max[i][j]);
        printf("\n");
    }
    printf("\n");

    // Need
    printf("Need:\n");
    for (j = 0; j < num_resources; j++)
        printf("   R%d", j);
    printf("\n");
    for (i = 0; i < num_procs; i++) {
        printf("P%d:", i);
        for (j = 0; j < num_resources; j++)
            printf(" %d ", state->max[i][j] - state->allocated[i][j]);
        printf("\n");
    }

    printf("##########################\n");
}
