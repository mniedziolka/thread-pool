#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "future.h"

#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

future_t** future;

static void* fun(void* arg, size_t argsz, size_t* resultsz) {
    uint64_t* k = arg;
    *k *= (*k + 3);
    return k;
}

int32_t main() {
    uint32_t n;
    scanf("%u", &n);

    if (n < 3) { printf("%u\n", n); return 0; }

    thread_pool_t* pool = malloc(sizeof(thread_pool_t));
    if (pool == NULL) {
        fprintf(stderr, "ERROR: threadpool malloc failed\n");
        return -1;
    }

    thread_pool_init(pool, 3);



    future = malloc(MAX(n, 3) * sizeof(future_t*));
    if (future == NULL) {
        fprintf(stderr, "ERROR: future malloc failed\n");
        return -1;
    }

    for (uint32_t i = 0; i < n; ++i) {
        future[i] = malloc(sizeof(future_t));
        if (future[i] == NULL) {
            fprintf(stderr, "ERROR: future malloc failed\n");
            return -1;
        }
    }
    int32_t err = 0;

    for (uint32_t i = 0; i < 3; ++i) {
        uint64_t* k = malloc(sizeof(uint64_t));
        if (k == NULL) {
            fprintf(stderr, "ERROR: result malloc failed\n");
            return -1;
        }
        *k = i + 1;
        future[i]->result = k;
        future[i]->result_size = sizeof(uint64_t);
        sem_init(&future[i]->finished, 0, 1);
    }

    for (uint32_t i = 3; i < n; ++i) {
        if ((err = map(pool, future[i], future[i - 3], fun)) != 0) {
            fprintf(stderr, "ERROR: map failed\n");
            return err;
        }
    }

    uint64_t* result1 = await(future[n - 3]);
    uint64_t* result2 = await(future[n - 2]);
    uint64_t* result3 = await(future[n - 1]);
    for (uint32_t i = 0; i < n - 3; ++i) { await(future[i]); }
    printf("%lu\n", *result1 * *result2 * *result3);

    thread_pool_destroy(pool);
    free(result1);
    free(result2);
    free(result3);
    for (uint32_t i = 0; i < n; ++i) {
        free(future[i]);
    }

    free(future);
    free(pool);

    return 0;
}