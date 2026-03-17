#include "tus.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// this is a sample application that is using tsl library to
// create and work with threads.

#define MAXCOUNT 1000
#define YIELDPERIOD 100

void *worker_a();
void *worker_b();
void *worker_c();
void *worker_d();
void *worker_e();
void *worker_f();

void *foo(void *v) {
    int count = 1;
    int mytid;

    mytid = tus_gettid();
    printf("thread %d started running (first time);  at the start function\n", mytid);

    // while (count < MAXCOUNT) {
    while (1) {
        printf("thread %d is running (count=%d)\n", mytid, count);
        if (count % YIELDPERIOD == 0) {
            tus_yield(TUS_ANY);
        }
        count++;
        if (count == MAXCOUNT)
            break;
    }
    return (NULL);
}

typedef struct {
    int a;
    int b;
    char *str;
} test_struct;

void *argument_passing(void *arg) {
    test_struct *test = arg;
    printf("a: %d, b:%d, str:%s\n", test->a, test->b, test->str);
}

void *worker_a() {
    int b = tus_create_thread(worker_b, NULL);
    int ret = tus_yield(b);
    assert(b == ret);
    printf("worker_a\n");
    tus_exit();
    return 0;
}

void *worker_b() {
    int c = tus_create_thread(worker_c, NULL);
    int ret = tus_yield(c);
    assert(c == ret);
    printf("worker_b\n");
    tus_exit();
    return 0;
}

void *worker_c() {
    printf("worker_c\n");
    return 0;
}
int e;
void *worker_d() {
    e = tus_create_thread(worker_e, NULL);
    int ret = tus_join(e);
    assert(e == ret);
    printf("worker_d\n");
    tus_exit();
    return 0;
}

void *worker_e() {
    int f = tus_create_thread(worker_f, NULL);
    printf("yielding from e to f\n");
    int ret = tus_yield(f);
    ret = tus_join(f);
    assert(f == ret);
    printf("worker_e\n");
    tus_exit();
    return 0;
}

void *worker_f() {
    printf("yielding from f to e\n");
    tus_yield(e);
    printf("worker_f\n");
    tus_exit();
    return 0;
}

void test_main_thread_exit() {
    test_struct t;
    t.a = 10;
    t.b = 5;
    t.str = "Hello world";
    int tid1 = tus_create_thread(argument_passing, &t);
    tus_yield(tid1);
    tus_exit();
}

int main(int argc, char **argv) {
    tus_init(ALG_FCFS);
    // test_main_thread_exit();
    // int a = tus_create_thread(worker_a, NULL);
    // tus_yield(a);
    // int d = tus_create_thread(worker_d, NULL);
    // int ret = tus_join(d);
    // assert(d == ret);
    // tus_exit();
    // exit(0);
    int *tids;
    int i;
    int numthreads = 0;

    if (argc != 2) {
        printf("usage: ./app numofthreads\n");
        exit(1);
    }

    numthreads = atoi(argv[1]);

    tids = (int *)malloc(numthreads * sizeof(int));

    tids[0] = tus_init(ALG_FCFS);
    // at tid[0] we have the id of the main thread

    for (i = 1; i < numthreads; ++i) {
        tids[i] = tus_create_thread((void *)&foo, NULL);
        printf("thead %d created\n", (int)tids[i]);
    }

    for (i = 1; i < numthreads; ++i) {
        printf("main: waiting for thead %d\n", (int)tids[i]);
        tus_join(tids[i]);
        printf("main: thead %d finished\n", (int)tids[i]);
    }

    printf("main thread calling tlib_exit\n");
    tus_exit();

    return 0;
}
