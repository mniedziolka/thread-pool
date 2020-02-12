/** @file
 * Interfejs klasy przechowującej miasto
 *
 * @author Michał Niedziółka <michal.niedziolka@students.mimuw.edu.pl>
 * @copyright Michał Niedziółka
 * @date 11.02.2020
 */

#ifndef FUTURE_H
#define FUTURE_H

#include "../threadpool/threadpool.h"

/**
 * Callable function.
 */
typedef struct callable {
  void *(*function)(void*, size_t, size_t*); ///< f(arg, argsz, result_size);
  void* arg; ///<                                         array of arguments;
  size_t argsz; ///<                                     number of arguments;
} callable_t;

/**
 * Future that will get the value asynchronously
 */
typedef struct future {
    callable_t callable;
    sem_t finished;
    void* result;
    size_t result_size;
} future_t;

int async(thread_pool_t* pool, future_t* future, callable_t callable);

int map(thread_pool_t* pool, future_t* future, future_t* from,
        void* (*function)(void*, size_t, size_t*));

void *await(future_t *future);

#endif
