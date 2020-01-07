#include "threadpool.h"

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>


// QUEUE FUNCTIONS BEGIN

static void push(queue_t* queue, runnable_t runnable) {
    node_t* new = malloc(sizeof(node_t));
    if (new == NULL) {
        fprintf (stderr, "ERROR: node_create failed\n");
        exit(-1);
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
        fprintf(stderr, "ERROR: pop from empty queue\n");
        exit(-1);
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

static void free_queue(queue_t* queue) {
    node_t* node = queue->first;
    while (node != NULL) {
        node_t* tmp = node;
        node = node->next;
        free(tmp);
    }
}

// QUEUE FUNCTIONS END

struct {
    struct sigaction action;
    struct sigaction old_action;
    thread_pool_t** known_pools;
    size_t pools_size;
    unsigned last;
    sigset_t block_mask;
} handler;

static void sigint_destroy() {
    fprintf(stderr, "ERROR: SIGINT caught\n");
    for (unsigned i = 0; i < handler.last; ++i) {
        if (handler.known_pools[i] != NULL) {
            handler.known_pools[i]->finished = true;
        }
    }
    for (unsigned i = 0; i < handler.last; ++i) {
        if (handler.known_pools[i] != NULL) {
            thread_pool_destroy(handler.known_pools[i]);
        }
    }
}

__attribute__((constructor))
static void init_handler() {
    sigemptyset(&handler.block_mask);
    sigaddset(&handler.block_mask, SIGINT);
    handler.action.sa_sigaction = sigint_destroy;
    handler.action.sa_flags = SA_SIGINFO;
    handler.action.sa_mask = handler.block_mask;
    int err = sigaction(SIGINT, &handler.action, &handler.old_action);
    if (err != 0) {
        fprintf(stderr, "ERROR: signal init failed\n");
        exit(err);
    }
    handler.pools_size = 4;
    handler.last = 0;
    handler.known_pools = malloc(4 * sizeof(thread_pool_t*));
}

__attribute__((destructor))
static void destroy_handler() {
    free(handler.known_pools);
}

static void* thread_function(void* arg) {
    pthread_sigmask(SIG_BLOCK, &handler.block_mask, NULL);
    thread_pool_t* pool = arg;
    int* err = malloc(sizeof(int));
    while (true) {
        *err = sem_wait(&pool->waiting_threads);
        if (*err != 0) {
            fprintf(stderr, "ERROR: sem_wait failed\n");
            return err;
        }

        *err = sem_wait(&pool->mutex);
        if (*err != 0) {
            fprintf(stderr, "ERROR: sem_wait failed\n");
            return err;
        }
        // BEGIN CRITICAL SECTION

        if (pool->queue->size == 0) {
            *err = sem_post(&pool->mutex);
            if (*err != 0) {
                fprintf(stderr, "ERROR: sem_post failed\n");
                return err;
            }
            *err = 0;
            return err;
        }
        runnable_t task = pop(pool->queue);

        // END CRITICAL SECTION
        *err = sem_post(&pool->mutex);
        if (*err != 0) {
            fprintf(stderr, "ERROR: sem_post failed\n");
            return err;
        }

        (*task.function)(task.arg, task.argsz);
    }
}

int thread_pool_init(thread_pool_t* pool, size_t num_threads) {
    // INIT QUEUE
    pool->queue = malloc(sizeof(queue_t));
    if (pool->queue == NULL) {
        fprintf(stderr, "ERROR: queue_create failed\n");
        return -1;
    }
    pool->queue->size = 0;
    pool->queue->first = NULL;
    pool->queue->last = NULL;

    // INIT SEMAPHORES
    int err = sem_init(&pool->mutex, 0, 1);
    if (err != 0) {
        fprintf(stderr, "ERROR: sem_init failed\n");
        return -1;
    }

    err = sem_init(&pool->waiting_threads, 0, 0);
    if (err != 0) {
        fprintf(stderr, "ERROR: sem_init failed\n");
        return -1;
    }

    // INIT ATTRIBUTE
    err = pthread_attr_init(&pool->attr);
    if (err != 0) {
        fprintf(stderr, "ERROR: attr_init failed");
        return err;
    }

    err = pthread_attr_setdetachstate(&pool->attr,PTHREAD_CREATE_JOINABLE);
    if (err != 0) {
        fprintf(stderr, "ERROR: attr_setdetachstate failed");
        return err;
    }

    // INIT THREADS
    pool->pool_size = num_threads;

    pool->threads = malloc(num_threads * sizeof(pthread_t));
    if (pool->threads == NULL) {
        fprintf(stderr, "ERROR: threads array malloc failed\n");
        return -1;
    }

    for (unsigned i = 0; i < num_threads; ++i) {
        err = pthread_create(&pool->threads[i], &pool->attr, thread_function, pool);
        if (err != 0) {
            fprintf(stderr, "ERROR: pthread_create failed\n");
            return err;
        }
    }

    if (handler.last == handler.pools_size) {
        thread_pool_t** new_pools = realloc(handler.known_pools,
                2 * handler.pools_size * sizeof(thread_pool_t*));
        if (new_pools == NULL) {
            return -1;
        }
        handler.pools_size *= 2;
        handler.known_pools = new_pools;
    }
    handler.known_pools[handler.last++] = pool;

    // INIT FINISHED
    pool->finished = false;

    return 0;
}

void thread_pool_destroy(thread_pool_t* pool) {
    for (unsigned i = 0; i < handler.last; ++i) {
        if (pool == handler.known_pools[i]) {
            handler.known_pools[i] = NULL;
            break;
        }
    }
    int err = sem_wait(&pool->mutex);
    if (err != 0) {
        fprintf(stderr, "ERROR: sem_wait failed\n");
        exit(err);
    }
    // BEGIN CRITICAL SECTION

    pool->finished = true;
    for (unsigned i = 0; i < pool->pool_size + pool->queue->size; ++i) {
        sem_post(&pool->waiting_threads);
    }

    // END CRITICAL SECTION
    err = sem_post(&pool->mutex);
    if (err != 0) {
        fprintf(stderr, "ERROR: sem_post failed\n");
        exit(err);
    }

    void* retval;
    for (unsigned i = 0; i < pool->pool_size; ++i) {
        pthread_join(pool->threads[i], &retval);
        int* ret = retval;
        if (*ret != 0) {
            fprintf(stderr, "ERROR: Thread exited with %d\nYou probably sent signal during destroy", *ret);
            free(ret);
            exit(-1);
        }
        free(ret);
    }

    err = pthread_attr_destroy(&pool->attr);
    if (err != 0) {
        fprintf(stderr, "pthread_attr_destroy failed\n");
        exit(-1);
    }

    err = sem_destroy(&pool->mutex);
    if (err != 0) {
        fprintf(stderr, "sem_destroy failed\n");
        exit(-1);
    }
    err = sem_destroy(&pool->waiting_threads);
    if (err != 0) {
        fprintf(stderr, "sem_destroy failed\n");
        exit(-1);
    }

    free_queue(pool->queue);
    free(pool->queue);
    free(pool->threads);


}

int defer(struct thread_pool* pool, runnable_t runnable) {
    int err = sem_wait(&pool->mutex);
    if (err != 0) {
        fprintf(stderr, "ERROR: sem_wait failed\n");
        return err;
    }
    // BEGIN CRITICAL SECTION

    if (pool->finished) { return -1; }
    push(pool->queue, runnable);

    // END CRITICAL SECTION
    err = sem_post(&pool->mutex);
    if (err != 0) {
        fprintf(stderr, "ERROR: sem_post failed\n");
        return err;
    }

    err = sem_post(&pool->waiting_threads);
    if (err != 0) {
        fprintf(stderr, "ERROR: sem_post failed\n");
        return err;
    }

    return 0;
}
