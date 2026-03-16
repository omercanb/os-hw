#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <assert.h>

/* We want the extra information from these definitions */
#ifndef __USE_GNU
#define __USE_GNU
#endif /* __USE_GNU */

#include <ucontext.h>
#include "tus.h"

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

int tus_init(int salg);
int tus_create_thread (void *(*tsf)(void *), void *targ);
int tus_yield (int tid);
int tus_exit();
int tus_join(int tid);
int tus_cancel(int tid);
int tus_gettid();


int
tus_init ( int salg)
{
    return (0);
    // we put return(0) as a placeholder.
    // read project about what to return.
    //and change return() accordingly.
}



int
tus_create_thread(void *(*tsf)(void *), void *targ)
{
    return (0);
}



int
tus_yield(int tid)
{
    return (0);
}


void
tus_exit()
{
    return;
}

int
tus_join(int tid)
{
    return (0);
}


int
tus_cancel(int tid)
{
    return (0);
}


int
tus_gettid()
{
    return (0);
}

