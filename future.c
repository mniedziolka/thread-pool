#include <stdio.h>
#include "future.h"

typedef void *(*function_t)(void *);

static void fun(void* arg, size_t size) {
    future_t* f = arg;
    f->result = f->callable.function(f->callable.arg, f->callable.argsz, &f->result_size);
    sem_post(&f->finished);
}

int async(thread_pool_t *pool, future_t *future, callable_t callable) {
    future->callable = callable;
    int err = sem_init(&future->finished, 0, 0);
    if (err != 0) {
        fprintf(stderr, "ERROR: sem_init failed\n");
        return -1;
    }

    runnable_t r;
    r.function = fun;
    r.arg = future;
    r.argsz = sizeof(future_t);

    defer(pool, r);

    return 0;
}

int map(thread_pool_t* pool, future_t* future, future_t* from,
        void *(*function)(void *, size_t, size_t*)) {
  return 0;
}

void* await(future_t* future) {
    sem_wait(&future->finished);
    sem_destroy(&future->finished);

    return future->result;
}
