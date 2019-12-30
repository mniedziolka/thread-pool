#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <stddef.h>
#include <semaphore.h>

typedef struct runnable {
  void (*function)(void *, size_t);
  void *arg;
  size_t argsz;
} runnable_t;

typedef struct node {
    int data;
    struct node *next;
} node_t;

typedef struct queue {

} queue_t;

typedef struct thread_pool {
    size_t pool_size;
    sem_t mutex;
    sem_t waiting_threads;
    queue_t * queue;
} thread_pool_t;

int thread_pool_init(thread_pool_t *pool, size_t pool_size);

void thread_pool_destroy(thread_pool_t *pool);

int defer(thread_pool_t *pool, runnable_t runnable);

#endif
