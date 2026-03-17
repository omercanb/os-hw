// // #include "tus.h"
// // #include <assert.h>
// // #include <stdio.h>
// // #include <stdlib.h>
// // #include <unistd.h>
// //
// // // this is a sample application that is using tsl library to
// // // create and work with threads.
// //
// // #define MAXCOUNT 5
// // #define YIELDPERIOD 1
// //
// // void *worker_a();
// // void *worker_b();
// // void *worker_c();
// // void *worker_d();
// // void *worker_e();
// // void *worker_f();
// //
// // // Things to test
// // // yields
// // // joins
// // // cancels
// // // scheduling algos
// // // argument passing and stacks
// //
// // void *foo(void *v) {
// //     int count = 1;
// //     int mytid;
// //
// //     mytid = tus_gettid();
// //     printf("thread %d started running (first time);  at the start function\n", mytid);
// //
// //     // while (count < MAXCOUNT) {
// //     while (1) {
// //         printf("thread %d is running (count=%d)\n", mytid, count);
// //         if (count % YIELDPERIOD == 0) {
// //             tus_yield(TUS_ANY);
// //         }
// //         count++;
// //         if (count == MAXCOUNT)
// //             break;
// //     }
// //     return (NULL);
// // }
// //
// // typedef struct {
// //     int a;
// //     int b;
// //     char *str;
// // } test_struct;
// //
// // void *argument_passing(void *arg) {
// //     test_struct *test = arg;
// //     printf("a: %d, b:%d, str:%s\n", test->a, test->b, test->str);
// // }
// //
// // void *worker_a() {
// //     int b = tus_create_thread(worker_b, NULL);
// //     int ret = tus_yield(b);
// //     assert(b == ret);
// //     printf("worker_a\n");
// //     tus_exit();
// //     return 0;
// // }
// //
// // void *worker_b() {
// //     int c = tus_create_thread(worker_c, NULL);
// //     int ret = tus_yield(c);
// //     assert(c == ret);
// //     printf("worker_b\n");
// //     tus_exit();
// //     return 0;
// // }
// //
// // void *worker_c() {
// //     printf("worker_c\n");
// //     return 0;
// // }
// // int e;
// // void *worker_d() {
// //     e = tus_create_thread(worker_e, NULL);
// //     int ret = tus_join(e);
// //     assert(e == ret);
// //     printf("worker_d\n");
// //     tus_exit();
// //     return 0;
// // }
// //
// // void *worker_e() {
// //     int f = tus_create_thread(worker_f, NULL);
// //     printf("yielding from e to f\n");
// //     int ret = tus_yield(f);
// //     ret = tus_join(f);
// //     assert(f == ret);
// //     printf("worker_e\n");
// //     tus_exit();
// //     return 0;
// // }
// //
// // void *worker_f() {
// //     printf("yielding from f to e\n");
// //     tus_yield(e);
// //     printf("worker_f\n");
// //     tus_exit();
// //     return 0;
// // }
// //
// // void test_main_thread_exit() {
// //     test_struct t;
// //     t.a = 10;
// //     t.b = 5;
// //     t.str = "Hello world";
// //     int tid1 = tus_create_thread(argument_passing, &t);
// //     tus_yield(tid1);
// //     tus_exit();
// // }
// //
// // int main(int argc, char **argv) {
// //     tus_init(ALG_FCFS);
// //     // test_main_thread_exit();
// //     // int a = tus_create_thread(worker_a, NULL);
// //     // tus_yield(a);
// //     // int d = tus_create_thread(worker_d, NULL);
// //     // int ret = tus_join(d);
// //     // assert(d == ret);
// //     // tus_exit();
// //     // exit(0);
// //     int *tids;
// //     int i;
// //     int numthreads = 0;
// //
// //     if (argc != 2) {
// //         printf("usage: ./app numofthreads\n");
// //         exit(1);
// //     }
// //
// //     numthreads = atoi(argv[1]);
// //
// //     tids = (int *)malloc(numthreads * sizeof(int));
// //
// //     tids[0] = tus_init(ALG_FCFS);
// //     // at tid[0] we have the id of the main thread
// //
// //     for (i = 1; i < numthreads; ++i) {
// //         tids[i] = tus_create_thread((void *)&foo, NULL);
// //         printf("thead %d created\n", (int)tids[i]);
// //     }
// //
// //     for (i = 1; i < numthreads; ++i) {
// //         printf("main: waiting for thead %d\n", (int)tids[i]);
// //         tus_join(tids[i]);
// //         printf("main: thead %d finished\n", (int)tids[i]);
// //     }
// //
// //     printf("main thread calling tlib_exit\n");
// //     tus_exit();
// //
// //     return 0;
// // }
// //
//
// #include "tus.h"
// #include <assert.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
//
// // this is a sample application that is using tsl library to
// // create and work with threads.
//
// #define MAXCOUNT 5
// #define YIELDPERIOD 1
//
// void *worker_a();
// void *worker_b();
// void *worker_c();
// void *worker_d();
// void *worker_e();
// void *worker_f();
//
// // Things to test
// // yields
// // joins
// // cancels
// // scheduling algos
// // argument passing and stacks
//
// void *foo(void *v) {
//     int count = 1;
//     int mytid;
//
//     mytid = tus_gettid();
//     printf("thread %d started running (first time);  at the start function\n", mytid);
//
//     // while (count < MAXCOUNT) {
//     while (1) {
//         printf("thread %d is running (count=%d)\n", mytid, count);
//         if (count % YIELDPERIOD == 0) {
//             tus_yield(TUS_ANY);
//         }
//         count++;
//         if (count == MAXCOUNT)
//             break;
//     }
//     return (NULL);
// }
//
// typedef struct {
//     int a;
//     int b;
//     char *str;
// } test_struct;
//
// void *argument_passing(void *arg) {
//     test_struct *test = arg;
//     printf("a: %d, b:%d, str:%s\n", test->a, test->b, test->str);
// }
//
// void *worker_a() {
//     int b = tus_create_thread(worker_b, NULL);
//     int ret = tus_yield(b);
//     assert(b == ret);
//     printf("worker_a\n");
//     tus_exit();
//     return 0;
// }
//
// void *worker_b() {
//     int c = tus_create_thread(worker_c, NULL);
//     int ret = tus_yield(c);
//     assert(c == ret);
//     printf("worker_b\n");
//     tus_exit();
//     return 0;
// }
//
// void *worker_c() {
//     printf("worker_c\n");
//     return 0;
// }
// int e;
// void *worker_d() {
//     e = tus_create_thread(worker_e, NULL);
//     int ret = tus_join(e);
//     assert(e == ret);
//     printf("worker_d\n");
//     tus_exit();
//     return 0;
// }
//
// void *worker_e() {
//     int f = tus_create_thread(worker_f, NULL);
//     printf("yielding from e to f\n");
//     int ret = tus_yield(f);
//     ret = tus_join(f);
//     assert(f == ret);
//     printf("worker_e\n");
//     tus_exit();
//     return 0;
// }
//
// void *worker_f() {
//     printf("yielding from f to e\n");
//     tus_yield(e);
//     printf("worker_f\n");
//     tus_exit();
//     return 0;
// }
//
// void test_main_thread_exit() {
//     test_struct t;
//     t.a = 10;
//     t.b = 5;
//     t.str = "Hello world";
//     int tid1 = tus_create_thread(argument_passing, &t);
//     tus_yield(tid1);
//     tus_exit();
// }
//
// int main(int argc, char **argv) {
//     tus_init(ALG_FCFS);
//     // test_main_thread_exit();
//     // int a = tus_create_thread(worker_a, NULL);
//     // tus_yield(a);
//     // int d = tus_create_thread(worker_d, NULL);
//     // int ret = tus_join(d);
//     // assert(d == ret);
//     // tus_exit();
//     // exit(0);
//     int *tids;
//     int i;
//     int numthreads = 0;
//
//     if (argc != 2) {
//         printf("usage: ./app numofthreads\n");
//         exit(1);
//     }
//
//     numthreads = atoi(argv[1]);
//
//     tids = (int *)malloc(numthreads * sizeof(int));
//
//     tids[0] = tus_init(ALG_FCFS);
//     // at tid[0] we have the id of the main thread
//
//     for (i = 1; i < numthreads; ++i) {
//         tids[i] = tus_create_thread((void *)&foo, NULL);
//         printf("thead %d created\n", (int)tids[i]);
//     }
//
//     for (i = 1; i < numthreads; ++i) {
//         printf("main: waiting for thead %d\n", (int)tids[i]);
//         tus_join(tids[i]);
//         printf("main: thead %d finished\n", (int)tids[i]);
//     }
//
//     printf("main thread calling tlib_exit\n");
//     tus_exit();
//
//     return 0;
// }

#include "tus.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAXCOUNT 5
#define YIELDPERIOD 1

void *foo(void *v) {
    int count = 1;
    int mytid;
    mytid = tus_gettid();
    printf("thread %d started running (first time);  at the start function\n", mytid);
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

// Test: argument passing
typedef struct {
    int a;
    int b;
    char *str;
} test_struct;

void *argument_passing(void *arg) {
    test_struct *test = arg;
    printf("a: %d, b:%d, str:%s\n", test->a, test->b, test->str);
    return NULL;
}

void test_argument_passing() {
    printf("=== TEST: argument passing ===\n");
    printf("EXPECTED OUTPUT:\n");
    printf("a: 10, b:5, str:Hello world\n");
    printf("ACTUAL OUTPUT:\n");

    test_struct t = {.a = 10, .b = 5, .str = "Hello world"};
    int tid = tus_create_thread(argument_passing, &t);
    tus_yield(tid);
    tus_exit();
}

// Test: yield chain (A -> B -> C, then unwinds)
void *yield_c();
void *yield_b();

void *yield_a() {
    int b = tus_create_thread(yield_b, NULL);
    int ret = tus_yield(b);
    assert(b == ret);
    printf("yield_a\n");
    tus_exit();
    return NULL;
}

void *yield_b() {
    int c = tus_create_thread(yield_c, NULL);
    int ret = tus_yield(c);
    assert(c == ret);
    printf("yield_b\n");
    tus_exit();
    return NULL;
}

void *yield_c() {
    printf("yield_c\n");
    return NULL;
}

void test_yield_chain() {
    printf("=== TEST: yield chain ===\n");
    printf("EXPECTED OUTPUT:\n");
    printf("  yield_c\n");
    printf("  yield_b\n");
    printf("  yield_a\n");
    printf("ACTUAL OUTPUT:\n");

    int a = tus_create_thread(yield_a, NULL);
    tus_yield(a);
    tus_exit();
}

// Test: join chain (A joins B, B joins C)
int join_b_tid;
void *join_c();
void *join_b();

void *join_a() {
    join_b_tid = tus_create_thread(join_b, NULL);
    int ret = tus_join(join_b_tid);
    assert(join_b_tid == ret);
    printf("join_a\n");
    tus_exit();
    return NULL;
}

void *join_b() {
    int c = tus_create_thread(join_c, NULL);
    printf("yielding from join_b to join_c\n");
    int ret = tus_yield(c);
    ret = tus_join(c);
    assert(c == ret);
    printf("join_b\n");
    tus_exit();
    return NULL;
}

void *join_c() {
    printf("yielding from join_c to join_b\n");
    tus_yield(join_b_tid);
    printf("join_c\n");
    tus_exit();
    return NULL;
}

void test_join_chain() {
    printf("=== TEST: join chain ===\n");
    printf("EXPECTED OUTPUT:\n");
    printf("  yielding from join_b to join_c\n");
    printf("  yielding from join_c to join_b\n");
    printf("  join_c\n");
    printf("  join_b\n");
    printf("  join_a\n");
    printf("ACTUAL OUTPUT:\n");

    int a = tus_create_thread(join_a, NULL);
    int ret = tus_join(a);
    assert(a == ret);
    tus_exit();
}

// Test: cancel
void *cancel_target(void *arg) {
    printf("cancel_target: running (should NOT reach second print)\n");
    tus_yield(TUS_ANY);
    printf("cancel_target: resumed after yield (BAD - should have been cancelled)\n");
    return NULL;
}

void test_cancel() {
    printf("=== TEST: cancel ===\n");
    printf("EXPECTED OUTPUT:\n");
    printf("  cancel_target: running (should NOT reach second print)\n");
    printf("  cancel returned: 0\n");
    printf("  main: done\n");
    printf("ACTUAL OUTPUT:\n");

    int tid = tus_create_thread(cancel_target, NULL);
    tus_yield(tid);
    int ret = tus_cancel(tid);
    printf("cancel returned: %d\n", ret);
    printf("main: done\n");
    tus_exit();
}

// Test: N threads with foo
void test_n_threads(int numthreads) {
    printf("=== TEST: %d threads running foo ===\n", numthreads);
    int *tids = malloc(numthreads * sizeof(int));
    for (int i = 0; i < numthreads; i++) {
        tids[i] = tus_create_thread((void *)&foo, NULL);
        printf("thread %d created\n", tids[i]);
    }
    for (int i = 0; i < numthreads; i++) {
        printf("main: waiting for thread %d\n", tids[i]);
        tus_join(tids[i]);
        printf("main: thread %d finished\n", tids[i]);
    }
    free(tids);

    printf("main thread calling tus_exit\n");
    tus_exit();
}

// ============================================================
// main
// ============================================================
int main(int argc, char **argv) {
    tus_init(ALG_FCFS);

    if (argc == 2) {
        int numthreads = atoi(argv[1]);
        test_n_threads(numthreads);
    } else {
        // Uncomment one test at a time:
        test_argument_passing();
        test_yield_chain();
        test_join_chain();
        test_cancel();
    }

    return 0;
}
