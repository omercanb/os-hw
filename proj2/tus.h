#ifndef _TUS_H_
#define _TUS_H_

// do not change this header file (tsl.h)
// it is the interface of the tsl library to the applications

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
                      //

int tus_init(int salg);
int tus_create_thread(void *(*tsf)(void *), void *targ);
int tus_yield(int tid);
void tus_exit();
int tus_join(int tid);
int tus_cancel(int tid);
int tus_gettid();

#endif
