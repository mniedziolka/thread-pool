#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "threadpool.h"

typedef struct value_time_row {
    uint32_t v, t, r;
} value_time_row_t;

sem_t* guard;
uint64_t* result;

static void fun(void* arg, size_t size __attribute__((unused))) {
    value_time_row_t* context = arg;
    usleep(context->t * 1000);
    sem_wait(&guard[context->r]);
    result[context->r] += context->v;
    sem_post(&guard[context->r]);
    free(context);
}

int32_t main() {
    uint32_t k, n;
    scanf("%u%u", &k, &n);
    guard = malloc(k * sizeof(sem_t));
    if (guard == NULL) {
        fprintf(stderr, "ERROR: sem alloc failed\n");
        return -1;
    }
    result = malloc(k * sizeof(uint64_t));
    if (result == NULL) {
        fprintf(stderr, "ERROR: sem alloc failed\n");
        return -1;
    }

    for (uint32_t i = 0; i < k; ++i) {
        result[i] = 0;
        sem_init(&guard[i], 0, 1);
    }

    thread_pool_t* pool = malloc(sizeof(thread_pool_t));
    if (pool == NULL) {
        fprintf(stderr, "ERROR: malloc has failed\n");
        return -1;
    }

    int32_t err = 0;
    if ((err = thread_pool_init(pool, 4)) != 0) {
        fprintf(stderr, "ERROR: thread init failed with %d\n", err);
        return err;
    }

    for (uint32_t i = 0; i < k * n; ++i) {
        uint32_t v, t;
        scanf("%u%u", &v, &t);

        value_time_row_t* context = malloc(sizeof(value_time_row_t));
        context->v = v;
        context->t = t;
        context->r = i / n;

        runnable_t r;
        r.function = fun;
        r.arg = context;
        r.argsz = sizeof(value_time_row_t);

        defer(pool, r);
    }

    thread_pool_destroy(pool);

    for (uint32_t i = 0; i < k; ++i) {
        printf("%lu\n", result[i]);
    }

    free(guard);
    free(result);
    free(pool);
}