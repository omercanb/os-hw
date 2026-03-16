#include <assert.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* We want the extra information from these definitions */

#ifndef __USE_GNU
#define __USE_GNU
#endif
#include "tus.h"
#include <ucontext.h>

#define TUS_MAXTHREADS \
    64 // maximum number of threads (including the main thread) that an
       // application can have.
#define TUS_STACKSIZE \
    16384 // bytes, i.e., 32 KB. This is the stack size for a new thread.

#define ALG_FCFS 1
#define ALG_RANDOM 2
#define ALG_MYALGORITHM 3

#define TID_MAIN \
    1 // tid of the main tread. this id is reserved for main thread.

#define TUS_ANY 0 // yield to a thread selected with a scheduling alg.

#define TUS_ERROR -1  // there is an error in the function execution.
#define TUS_SUCCESS 0 // function execution success

// you will implement your library in this file.
// you can define your internal structures, macros here.
// such as: #define ...
// if you wish you can use another header file (not necessary).
// but you should not change the tsl.h header file.
// you can also define and use global variables here.
// you can define and use additional functions (as many as you wish)
// in this file; besides the tsl library functions desribed in the assignment.
// these additional functions will not be available for
// applications directly.
//

int tus_init(int salg);
int tus_create_thread(void *(*tsf)(void *), void *targ);
int tus_yield(int tid);
void tus_exit();
int tus_join(int tid);
int tus_cancel(int tid);
int tus_gettid();
void stub(void *(*tsf)(void *), void *targ);

enum state { RUNNING,
             WAITING,
             READY,
             TERMINATED };

typedef struct TCB {
    // id is the index in threads
    unsigned int state; // running / waiting / ready / terminated
    ucontext_t context; // cpu context
} TCB;

TCB *threads[TUS_MAXTHREADS];
int num_threads = 0;
int cur_tid = 0;

int tus_init(int salg) {
    // Put main as the first thread
    // Get this current context
    ucontext_t context;
    long ret;
    ret = getcontext(&context);
    if (ret) {
        return TUS_ERROR;
    }
    // char *stack = (char *)context.uc_mcontext.gregs[REG_RSP];
    TCB *tcb = calloc(1, sizeof(TCB));
    if (!tcb) {
        return TUS_ERROR;
    }
    tcb->state = RUNNING;
    tcb->context = context;
    threads[0] = tcb;
    cur_tid = 0;

    num_threads++;
    return 0;
}

int tus_create_thread(void *(*tsf)(void *), void *targ) {
    if (num_threads == TUS_MAXTHREADS) {
        return TUS_ERROR;
    }
    // Get this current context
    ucontext_t context;
    long ret;
    ret = getcontext(&context);
    if (ret) {
        return TUS_ERROR;
    }
    // Some parts of the context need to be modified before swapping to it
    // ip points to stub
    context.uc_mcontext.gregs[REG_RIP] = (long long)stub;
    // sp points to allocated stack
    char *stack = malloc(sizeof(char) * TUS_STACKSIZE);
    if (!stack) {
        return TUS_ERROR;
    }

    context.uc_mcontext.gregs[REG_RSP] = (long long)(stack + TUS_STACKSIZE - 1);
    // Put into rdi and rsi the first and second arguments for stub (these are this functions arguments)
    context.uc_mcontext.gregs[REG_RDI] = (long long)tsf;
    context.uc_mcontext.gregs[REG_RSI] = (long long)targ;
    // Create a TCB block
    TCB *tcb = calloc(1, sizeof(TCB));
    if (!tcb) {
        return TUS_ERROR;
    }
    int tid = num_threads;
    tcb->state = READY;
    tcb->context = context;
    threads[tid] = tcb;

    num_threads++;
    // set context with the new context
    // // TODO this bottom line will be removed
    // setcontext(&context);
    return tid;
}

void stub(void *(*tsf)(void *), void *targ) {
    // to align stack pointer
    __asm__ volatile("and $-16, %rsp");
    printf("Were in here\n");
    fflush(stdout);
    // new thread will start its execution here
    // then we will call the thread start function
    tsf(targ); // calling the thread start function
    // tsf will retun to here
    // now ask for termination
    tus_exit(); // terminate
}

int save_context(int tid, int state) {
    // Get this current context
    ucontext_t context;
    long ret;
    ret = getcontext(&context);
    if (ret) {
        return TUS_ERROR;
    }
    threads[tid]->context = context;
    threads[tid]->state = state;
}

int tus_yield(int tid) {
    assert(tid != 0);
    // Search for tcb with tid
    TCB *new_tcb = threads[tid];
    if (!new_tcb || new_tcb->state != READY) {
        return -1;
    }
    if (cur_tid == tid) {
        return -1;
    }
    int caller_tid = cur_tid;
    // Save context of calling thread to threads[tid]
    // cur thread eventually returns to save context
    save_context(caller_tid, READY);
    // cur tid = caller tid means this is the first execution
    // after it the cur tid will be set to the yielded thread or another one
    if (cur_tid == caller_tid) {
        // Load context of new_tcb
        new_tcb->state = RUNNING;
        setcontext(&new_tcb->context);
    }
    return tid;
}

void tus_exit() {
    threads[cur_tid]->state = TERMINATED;
    for (int i = 0; i < TUS_MAXTHREADS; i++) {
        if (!threads[i] || threads[i]->state != READY) {
            continue;
        }
        threads[i]->state = RUNNING;
        setcontext(&threads[i]->context);
    }
    // If execution reaches here this is the last runnable thread
    for (int i = 0; i < TUS_MAXTHREADS; i++) {
        if (!threads[i] && threads[i]->state == WAITING) {
            printf("Warning: thread %d is waiting at program termination.\n", i);
        }
    }
    exit(0);
    return;
}

int tus_join(int tid) {
    return (0);
}

int tus_cancel(int tid) {
    return (0);
}

int tus_gettid() {
    return (0);
}
