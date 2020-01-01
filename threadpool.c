#include "threadpool.h"

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#define SEGFAULT 11

static void push(queue_t* queue, runnable_t runnable) {
    node_t* new = malloc(sizeof(node_t));
    if (new == NULL) {
        fprintf(stderr, "Malloc has failed\n");
        exit(SEGFAULT);
    }

    new->runnable = runnable;
    new->next = NULL;

    if (queue->size == 0) {
        queue->first = new;
        queue->last = new;
    } else {
        queue->last->next = new;
        queue->last = new;
    }

    ++queue->size;
}

static runnable_t pop(queue_t* queue) {
    if (queue->size == 0) {
        fprintf(stderr, "Pop from empty queue\n");
        exit(SEGFAULT);
    }

    runnable_t result = queue->first->runnable;
    node_t* new_next = queue->first->next;

    free(queue->first);
    queue->first = new_next;
    --queue->size;

    if (queue->size == 0) {
        queue->last = NULL;
    }

    return result;
}

static void* thread_function(void* arg) {
    thread_pool_t* pool = arg;
    int* err = malloc(sizeof(int));
    while (true) {
        *err = sem_wait(&pool->waiting_threads);
        if (*err != 0) {
            fprintf(stderr, "Broken semaphore\n");
            return err;
        }

        *err = 0;
        if (pool->finished) return err;

        *err = sem_wait(&pool->mutex);
        // CRITICAL SECTION BEGIN

        if (*err != 0) {
            fprintf(stderr, "Broken semaphore\n");
            return err;
        }

        runnable_t task = pop(pool->queue);

        // CRITICAL SECTION END
        *err = sem_post(&pool->mutex);
        if (*err != 0) {
            fprintf(stderr, "Broken semaphore\n");
            return err;
        }

        (*task.function)(task.arg, task.argsz);
    }
}

int thread_pool_init(thread_pool_t* pool, size_t num_threads) {
    // INIT SEMAPHORES
    int err = sem_init(&pool->mutex, 0, 1);
    if (err != 0) {
        fprintf(stderr, "Semaphore is not initialized\n");
        return SEGFAULT;
    }

    err = sem_init(&pool->waiting_threads, 0, 0);
    if (err != 0) {
        fprintf(stderr, "Semaphore is not initialized\n");
        return SEGFAULT;
    }

    // INIT ATTRIBUTE
    err = pthread_attr_init(&pool->attr);
    if (err != 0) {
        fprintf(stderr, "attr_init failed");
        return err;
    }

    err = pthread_attr_setdetachstate(&pool->attr,PTHREAD_CREATE_JOINABLE);
    if (err != 0) {
        fprintf(stderr, "attr_setdetachstate failed");
        return err;
    }

    // INIT THREADS
    pool->pool_size = num_threads;

    pool->threads = malloc(num_threads * sizeof(pthread_t));
    if (pool->threads == NULL) {
        fprintf(stderr, "Threads cannot be initialized\n");
        return SEGFAULT;
    }

    for (unsigned i = 0; i < num_threads; ++i) {
        err = pthread_create(&pool->threads[i], &pool->attr, thread_function, pool);
        if (err != 0) {
            fprintf(stderr, "Threads cannot be created\n");
            return err;
        }
    }

    // INIT QUEUE
    pool->queue = malloc(sizeof(queue_t));
    if (pool->queue == NULL) {
        fprintf(stderr, "Queue cannot be created\n");
        return SEGFAULT;
    }

    pool->queue->size = 0;
    pool->queue->first = NULL;
    pool->queue->last = NULL;


    // INIT FINISHED
    pool->finished = false;

    return 0;
}

void thread_pool_destroy(struct thread_pool* pool) {
    pool->finished = true;

    for (unsigned i = 0; i < pool->pool_size; ++i) {
        sem_post(&pool->waiting_threads);
    }

    void* retval;
    for (unsigned i = 0; i < pool->pool_size; ++i) {
        pthread_join(pool->threads[i], &retval);
        if (*((int*)retval) != 0) {
            fprintf(stderr, "Thread exited with %d\n", *((int*)retval));
            exit(SEGFAULT);
        }
    }

    int err = pthread_attr_destroy(&pool->attr);
    if (err != 0) {
        fprintf(stderr, "pthread_attr_destroy failed\n");
        exit(SEGFAULT);
    }

    err = sem_destroy(&pool->mutex);
    if (err != 0) {
        fprintf(stderr, "sem_destroy failed\n");
        exit(SEGFAULT);
    }
    err = sem_destroy(&pool->waiting_threads);
    if (err != 0) {
        fprintf(stderr, "sem_destroy failed\n");
        exit(SEGFAULT);
    }
}

int defer(struct thread_pool* pool, runnable_t runnable) {
    int err = sem_wait(&pool->mutex);
    // CRITICAL SECTION BEGIN

    if (err != 0) {
        fprintf(stderr, "Broken semaphore\n");
        return err;
    }

    push(pool->queue, runnable);

    // CRITICAL SECTION END
    err = sem_post(&pool->mutex);
    if (err != 0) {
        fprintf(stderr, "Broken semaphore\n");
        return err;
    }

    err = sem_post(&pool->waiting_threads);
    if (err != 0) {
        fprintf(stderr, "Broken semaphore\n");
        return err;
    }

    return 0;
}
