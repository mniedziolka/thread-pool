# Thread-pool

This library contains the thread-pool and future
mechanism (known from C++ std::future) implemented in C.

## Details of thread-pool
```C
int thread_pool_init(thread_pool_t *pool, size_t pool_size);

void thread_pool_destroy(thread_pool_t *pool);

int defer(thread_pool_t *pool, runnable_t runnable);
```
The thread_pool_init call initiates the argument pointed to by pool as the new pool in which it will have
pool_size of serving threads to complete the task. Library correctness is only guaranteed if each pool created by
thread_pool_init is destroyed by calling thread_pool_destroy with an argument representing these pools.


A call to defer(pool, runnable) instructs the pool to execute the task described by the argument runnable.
Function arguments are passed by the args pointer. In the args field there is the length of the buffer available
for writing and reading located under this pointer.

The tasks commissioned by defer are concurrent and independent of each other as far as
it's possible. Pool size is the limit of concurrent tasks. The pot during its operation no
should have more threads than specified by the pool_size parameter. Created threads are kept alive
until thread_pool_destroy.

## Details of the future mechanism
```C
int async(thread_pool_t* pool, future_t *future, callable_t callable);

int map(thread_pool_t* pool, future_t* future, future_t* from,
        void* (*function)(void*, size_t, size_t*));

void* await(future_t *future);
```
Running async initializes memory for future, assigns callable to calculate the value in the pool
and returns it to future mechanism.

User can now:

* Wait for value to be calculated:
```C
void* result = await(future_value);
```

* Use some pool to calculate next future using previous one.
```C
err = map(pool2, mapped_value, future_value, function2);
```

## Details of matrix.c
This is the program that uses the thread-pool to calculate the row-sums in matrix.
The first two lines contain two numbers k and n (number of rows and columns).
Then the program should read k*n lines with data. Every line contains two numbers v and t.
Number v in line i (number of row is defined by calculating floor(i/n)) specify the value.
Number t specify the time (in milliseconds) needed for calculating the v value.
Here is the example of correct input data:
```
2
3
1 2
1 5
12 4
23 9
3 11
7 2
```

Input date above represents matrix below.
```
|  1  1 12 |
| 23  3  7 |
```

Running the program:

```shell script
$ cat data1.dat | ./macierz
```
should have the same result as stated below
```
14
33
```

## Details of factorial.c
This is the program that will calculate the n! value using three threads and future mechanism:
Running:
```shell script
$ echo 5 | ./silnia
```
should result with
```
120
```

## Compiling
```shell script
mkdir build && cd build
cmake ..
make
```

Should compile the library to build/libasyncc.

## Running tests
You can run tests by

```shell script
make test
```

