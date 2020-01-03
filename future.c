#include <stdio.h>
#include <stdlib.h>
#include "future.h"

typedef void *(*function_t)(void *);

typedef struct pair_future {
    future_t* first;
    future_t* second;
} pair_future_t;

static void fun(void* arg, size_t size __attribute__((unused))) {
    future_t* f = arg;
    f->result = f->callable.function(f->callable.arg, f->callable.argsz, &f->result_size);
    sem_post(&f->finished);
}

static void fun_with_wait(void* arg, size_t size __attribute__((unused))) {
    pair_future_t* pair = arg;
    future_t* from = pair->first;
    sem_wait(&from->finished);
    future_t* to = pair->second;
    to->result = to->callable.function(from->result, from->result_size, &to->result_size);

    free(pair);
}


int async(thread_pool_t *pool, future_t *future, callable_t callable) {
    future->callable = callable;
    int err = sem_init(&future->finished, 1, 0);
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

    pair_future_t* pair = malloc(sizeof(pair_future_t*));
    if (pair == NULL) {
        fprintf(stderr, "ERROR: callable_init failed\n");
        return -1;
    }
    pair->first = from;
    pair->second = future;

    callable_t callable;
    callable.function = function;
    callable.arg = pair;

    future->callable = callable;
    int err = sem_init(&future->finished, 1, 0);
    if (err != 0) {
        fprintf(stderr, "ERROR: sem_init failed\n");
        return -1;
    }

    runnable_t r;
    r.function = fun_with_wait;
    r.arg = pair;
    r.argsz = sizeof(pair_future_t);

    defer(pool, r);

  return 0;
}

void* await(future_t* future) {
    sem_wait(&future->finished);
    sem_destroy(&future->finished);

    return future->result;
}
