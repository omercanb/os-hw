#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef __USE_GNU
#define __USE_GNU
#endif
#include <ucontext.h>

int n, m;

int
foo(long a, long b)
{
    ucontext_t con;

    long c = 12;
    long ret;
    
    ret = getcontext(&con);
    
    printf ("foo: address of local variable c=%lx, ret=%lx\n",
            (unsigned long)&c, (unsigned long)&ret);
    printf ("foo: address of arg a=%lx, arg b=%lx\n",
            (unsigned long)&a,
            (unsigned long)&b);
    printf("foo: contex REG_RSP value=%lx\n", (unsigned long) con.uc_mcontext.gregs[REG_RSP]);
    printf("foo: contex REG_RBP value=%lx\n", (unsigned long) con.uc_mcontext.gregs[REG_RBP]);
    return(13);
}


int
main(int argc, char **argv)
{
    
    ucontext_t con;
    long ret;
    
    printf("main: address of main is 0x%lx\n", (unsigned long)&main);
    printf("number of regs=%d\n", __NGREG);
    printf("REG_RBP index=%d\n", REG_RBP);
    printf("REG_RSP index=%d\n", REG_RSP);
    printf("REG_RIP index=%d\n", REG_RIP);

    
    printf ("\n");
    
    ret = getcontext(&con); // save the current cpu state into context structure con

    printf ("saved the context\n");
    
    printf("contex REG_RSP value=%lx\n", (unsigned long) con.uc_mcontext.gregs[REG_RSP]);
    printf("contex REG_RIP value=%lx\n", (unsigned long) con.uc_mcontext.gregs[REG_RIP]);
                                                              
                    
    printf ("addess of local variable ret=%lx\n", (unsigned long) &ret);
    
    ret = foo(39, 33);
    
}
