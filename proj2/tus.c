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

enum state { INVALID,
             RUNNING,
             WAITING,
             READY,
             TERMINATED };

typedef struct TCB {
    // id is the index in threads
    int tid;
    unsigned int state; // running / waiting / ready / terminated
    ucontext_t context; // cpu context
    int waited_for_by;  // the tid of the thread thats waited for with join
    char *stack;
    bool yielded;
} TCB;

int tus_init(int salg);
int tus_create_thread(void *(*tsf)(void *), void *targ);
int tus_yield(int tid);
void tus_exit();
int tus_join(int tid);
int tus_cancel(int tid);
int tus_gettid();
void stub(void *(*tsf)(void *), void *targ);
void switch_to(int tid);
void enqueue(TCB *thread);
int dequeue();
void queue_remove_index(int q_index);
bool queue_remove_thread(int tid);

void thread_add(TCB *thread);
TCB *thread_get(int tid);
void thread_remove(TCB *thread);
TCB *_threads[TUS_MAXTHREADS];
int num_threads = 0;

int cur_tid = 0;

bool initialized = false;

// The ready queue of tids
int tqueue[TUS_MAXTHREADS];
int num_queued = 0;

int scheduling_alg = ALG_FCFS;

int tus_init(int salg) {
    // Put main as the first thread
    // Get this current context
    scheduling_alg = salg;
    TCB *main_thread = calloc(1, sizeof(TCB));
    if (!main_thread) {
        return TUS_ERROR;
    }
    thread_add(main_thread);
    main_thread->state = RUNNING;
    cur_tid = TID_MAIN;
    num_threads = 1;
    initialized = true;
    // We don't need to save context of main at this point because
    // It either yields or joins saving it context, or it terminates without any need
    return 0;
}

int tus_create_thread(void *(*tsf)(void *), void *targ) {
    assert(initialized);
    if (num_threads == TUS_MAXTHREADS) {
        return TUS_ERROR;
    }
    // Create a TCB block
    TCB *thread = calloc(1, sizeof(TCB));
    if (!thread) {
        return TUS_ERROR;
    }
    // Get this current context
    // Some parts of the context need to be modified before swapping to it
    long ret = getcontext(&thread->context);
    if (ret) {
        return TUS_ERROR;
    }
    // sp points to allocated stack
    thread->stack = malloc(sizeof(char) * TUS_STACKSIZE);
    if (!thread->stack) {
        return TUS_ERROR;
    }
    // Stack pointer starts from large to small so we point to the end
    // Stack pointer needs to point past the bounds so we move up by a byte and allocate (start from size 0)
    thread->context.uc_mcontext.gregs[REG_RSP] = (long long)(thread->stack + TUS_STACKSIZE);

    // ip points to stub
    thread->context.uc_mcontext.gregs[REG_RIP] = (long long)stub;

    // Put into rdi and rsi the first and second arguments for stub (these are this functions arguments)
    thread->context.uc_mcontext.gregs[REG_RDI] = (long long)tsf;
    thread->context.uc_mcontext.gregs[REG_RSI] = (long long)targ;

    thread_add(thread);
    enqueue(thread);
    return thread->tid;
}

void stub(void *(*tsf)(void *), void *targ) {
    // to align stack pointer
    __asm__ volatile("and $-16, %rsp");
    // new thread will start its execution here
    // then we will call the thread start function
    tsf(targ); // calling the thread start function
    // tsf will retun to here
    // now ask for termination
    tus_exit(); // terminate
}

int tus_yield(int yielded_tid) {
    if (yielded_tid == TUS_ANY) {
        if (num_queued == 0) {
            return cur_tid;
        }
        yielded_tid = dequeue();
    }
    // Search for tcb with tid
    TCB *yielded_thread = thread_get(yielded_tid);
    if (!yielded_thread || yielded_thread->state != READY) {
        return -1;
    }
    // TODO check if this is in the pdf
    if (cur_tid == yielded_tid) {
        return cur_tid;
    }
    int caller_tid = cur_tid;
    TCB *caller = thread_get(caller_tid);
    // Save context of calling thread to threads[tid]
    // cur thread eventually returns to save context
    enqueue(caller);
    caller->yielded = false;
    long ret = getcontext(&caller->context);
    if (ret) {
        return TUS_ERROR;
    }
    // cur tid = caller tid means this is the first execution
    // after it the cur tid will be set to the yielded thread or another one
    if (!caller->yielded) {
        // Save that the thread has yielded
        caller->yielded = true;
        // Load context of new_tcb
        switch_to(yielded_tid);
        // Code never reaches here
    }

    return yielded_tid;
}

void tus_exit() {
    TCB *thread = thread_get(cur_tid);
    thread->state = TERMINATED;
    // There should be no running threads at this point
    for (int i = 0; i < TUS_MAXTHREADS; i++) {
        if (_threads[i]) {
            assert(_threads[i]->state != RUNNING);
            assert(_threads[i]->state != INVALID);
        }
    }
    if (thread->waited_for_by != -1) {
        TCB *thread_no_longer_waiting = thread_get(thread->waited_for_by);
        thread->waited_for_by = 1;
        enqueue(thread_no_longer_waiting);
    }
    if (num_queued) {
        switch_to(dequeue());
    } else {
        // Check if there are any running threads
        for (int i = 0; i < TUS_MAXTHREADS; i++) {
            if (_threads[i] && _threads[i]->state == WAITING) {
                printf("Warning: thread %d is waiting at program termination.\n", _threads[i]->tid);
            }
        }
        exit(0);
    }
}

int tus_join(int tid) {
    assert(tid != TID_MAIN);
    // Search for tcb with tid
    TCB *waited_thread = thread_get(tid);
    if (!waited_thread) {
        return -1;
    }
    if (waited_thread->state == TERMINATED) {
        thread_remove(waited_thread);
        return tid;
    }
    // The thread can't be waited for by another thread or it will be overwritten
    assert(waited_thread->waited_for_by == -1);

    // Return if joined on itself
    if (cur_tid == tid) {
        return tid;
    }

    int caller_tid = cur_tid;
    // Save context of calling thread to threads[tid]
    // cur thread eventually returns to save context
    TCB *caller = thread_get(cur_tid);
    caller->state = WAITING;
    waited_thread->waited_for_by = caller_tid;
    long ret;
    // Save the current context into caller
    ret = getcontext(&caller->context);
    if (ret) {
        return TUS_ERROR;
    }
    // cur tid = caller tid means this is the first execution
    // after it the cur tid will be set to the yielded thread or another one
    // If the waited for task is ready switch to it, if not schedule
    TCB *cur_thread = thread_get(cur_tid);
    // if (cur_thread && cur_thread->state == WAITING) {
    if (waited_thread->waited_for_by != -1) {
        // Always queue the caller thread
        enqueue(caller);
        if (waited_thread->state == READY) {
            // If the waited for thread is ready enqueue the caller and switch to the waited on thread
            switch_to(tid);
        } else {
            assert(num_queued);
            switch_to(dequeue());
        }
    }
    // Deallocate the waited thread
    // We assume we only one threads waits for a given threads
    printf("%d\n", waited_thread->state);
    assert(waited_thread->state == TERMINATED);
    assert(waited_thread->waited_for_by == -1);
    thread_remove(waited_thread);
    return tid;
}

// Swith to the thread removing it from the ready queue if it's there
void switch_to(int tid) {
    TCB *thread = thread_get(tid);
    assert(thread);
    queue_remove_thread(tid);
    thread->state = RUNNING;
    cur_tid = tid;
    setcontext(&thread->context);
}

int tus_cancel(int tid) {
    TCB *cancelled_thread = thread_get(tid);
    if (!cancelled_thread) {
        return -1;
    }
    // Tries to cancel itself
    if (cur_tid == tid) {
        return -1;
    }
    queue_remove_thread(tid);
    cancelled_thread->state = TERMINATED;

    return 0;
}

int tus_gettid() {
    return cur_tid;
}

// Makes the thread ready
void enqueue(TCB *thread) {
    assert(num_queued < TUS_MAXTHREADS);
    tqueue[num_queued] = thread->tid;
    thread->state = READY;
    num_queued++;
}

int dequeue() {
    assert(num_queued > 0);
    if (scheduling_alg == ALG_FCFS) {
        int tid = tqueue[0];
        queue_remove_index(0);
        return tid;
    } else if (scheduling_alg == ALG_RANDOM) {
        int rand_idx = rand() % num_queued;
        int tid = tqueue[rand_idx];
        queue_remove_index(rand_idx);
        return tid;
    } else {
        assert(false);
    }
}

void queue_remove_index(int q_index) {
    num_queued--;
    for (int i = q_index; i < num_queued; i++) {
        tqueue[i] = tqueue[i + 1];
    }
}

bool queue_remove_thread(int tid) {
    for (int i = 0; i < num_queued; i++) {
        if (tqueue[i] == tid) {
            queue_remove_index(i);
            return true;
        }
    }
    return false;
}

// Used when you create a new thread
// This thread should be ready to execute
void thread_add(TCB *tcb) {
    for (int i = 0; i < TUS_MAXTHREADS; i++) {
        if (_threads[i] == NULL) {
            _threads[i] = tcb;
            num_threads++;
            int tid = num_threads;
            // Threads are numbered 1 to n
            tcb->tid = tid;
            tcb->state = READY;
            tcb->waited_for_by = -1;
            tcb->yielded = true;
            break;
        }
    }
}

TCB *thread_get(int tid) {
    for (int i = 0; i < TUS_MAXTHREADS; i++) {
        if (_threads[i] && _threads[i]->tid == tid) {
            return _threads[i];
        }
    }
    return NULL;
}

void thread_remove(TCB *thread) {
    _threads[thread->tid] = NULL;
    queue_remove_thread(thread->tid);
    free(thread->stack);
    free(thread);
    num_threads--;
}
