/** @file
 * Future header file.
 *
 * @author Michał Niedziółka <michal.niedziolka@students.mimuw.edu.pl>
 * @copyright Michał Niedziółka
 * @date 11.02.2020
 */

#ifndef __FUTURE_H__
#define __FUTURE_H__

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
 * (similar to C++ std::future).
 */
typedef struct future {
    callable_t callable; ///<                 callable function;
    sem_t finished; ///< status of the future(pending/finished);
    void* result; ///<    pointer to the result of the function;
    size_t result_size; ///<                 size of the result;
} future_t;

/** @brief Create a future variable that will store the result of callable.
 * Create a future variable. Create runnable function
 * that will run callable function and add it to the thread-pool.
 * @param[in,out] pool    –                                pointer to the thread-pool;
 * @param[in,out] future  – pointer to a variable that will store the callable result;
 * @param[in] callable    –                  function that will be run by thread-pool;
 * @return @p 0, if future was created correctly
 * and callable function has been added to thread-pool.
 * Non-zero value, if errors occurred.
 */
int async(thread_pool_t* pool, future_t* future, callable_t callable);

/** @brief Create a future variable that will store the result of callable.
 * Create runnable function that will run callable function
 * and add a new task to the thread-pool.
 * @param[in,out] pool     – pointer to the thread-pool;
 * @param[in,out] future   – pointer to the future variable that stores the arguments
 *                                                                     to the function;
 * @param[in,out] from     – pointer to a variable that will store the callable result;
 * @param[in,out] function –                  function that will be run by thread-pool;
 * @return @p 0, if future variables were mapped correctly.
 * Non-zero value, if errors occurred.
 */
int map(thread_pool_t* pool, future_t* future, future_t* from,
        void* (*function)(void*, size_t, size_t*));

/** @brief Wait for future to finish.
 * Sleep on semaphore until the future is calculated.
 * @param[in,out] future  – pointer to a variable that will store the callable result;
 */
void *await(future_t *future);

#endif // __FUTURE_H__
