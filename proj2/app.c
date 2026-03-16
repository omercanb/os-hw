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

void *simple_worker(void *args) {
    char *str = (char *)args;
    printf("From test: %s\n", str);
    fflush(stdout);
    return 0;
}

void simple() {
    tus_create_thread(simple_worker, "Hello world");
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

void test_main_thread_exit() {
    test_struct t;
    t.a = 10;
    t.b = 5;
    t.str = "Hello world";
    int tid1 = tus_create_thread(argument_passing, &t);
    tus_exit();
}

int main(int argc, char **argv) {
    tus_init(0);
    // test_main_thread_exit();
    int a = tus_create_thread(worker_a, NULL);
    tus_yield(a);
    tus_exit();
    exit(0);
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
