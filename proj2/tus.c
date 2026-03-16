#include <assert.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* We want the extra information from these definitions */

#ifndef __USE_GNU
#define __USE_GNU
#endif
#include "tus.h"
#include <ucontext.h>

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
int num_threads = 0;

int tus_init(int salg);
int tus_create_thread(void *(*tsf)(void *), void *targ);
int tus_yield(int tid);
void tus_exit();
int tus_join(int tid);
int tus_cancel(int tid);
int tus_gettid();
void stub(void *(*tsf)(void *), void *targ);

int tus_init(int salg) {
    return (0);
    // we put return(0) as a placeholder.
    // read project about what to return.
    // and change return() accordingly.
}

int tus_create_thread(void *(*tsf)(void *), void *targ) {
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

    if (num_threads == TUS_MAXTHREADS) {
        return TUS_ERROR;
    }
    num_threads++;
    context.uc_mcontext.gregs[REG_RSP] = (long long)stack;
    // Put into rdi and rsi the first and second arguments for stub (these are this functions arguments)
    context.uc_mcontext.gregs[REG_RDI] = (long long)tsf;
    context.uc_mcontext.gregs[REG_RSI] = (long long)targ;
    // set context with the new context
    setcontext(&context);
    return 0;
}

void stub(void *(*tsf)(void *), void *targ) {
    // to align stack pointer
    printf("Were in here\n");
    __asm__ volatile("and $-16, %rsp");
    // new thread will start its execution here
    // then we will call the thread start function
    tsf(targ); // calling the thread start function
    // tsf will retun to here
    // now ask for termination
    tus_exit(); // terminate
}

int tus_yield(int tid) {
    return (0);
}

void tus_exit() {
    exit(1);
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
