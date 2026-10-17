
#ifndef COMMON_H
#define COMMON_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>

/* Return wall-clock time in seconds. */
static inline double GetTime(void)
{
    struct timeval t;
    int rc = gettimeofday(&t, NULL);
    assert(rc == 0);

    return (double)t.tv_sec + (double)t.tv_usec / 1000000.0;
}

/* Busy-wait for approximately howlong seconds. */
static inline void Spin(int howlong)
{
    double start = GetTime();

    while ((GetTime() - start) < (double)howlong) {
        /* Do nothing. */
    }
}

/* pthread_create wrapper: terminate on error. */
static inline void Pthread_create(
    pthread_t *thread,
    const pthread_attr_t *attr,
    void *(*start_routine)(void *),
    void *arg)
{
    int rc = pthread_create(thread, attr, start_routine, arg);

    if (rc != 0) {
        fprintf(stderr, "pthread_create failed: %d\n", rc);
        exit(EXIT_FAILURE);
    }
}

/* pthread_join wrapper: terminate on error. */
static inline void Pthread_join(pthread_t thread, void **value_ptr)
{
    int rc = pthread_join(thread, value_ptr);

    if (rc != 0) {
        fprintf(stderr, "pthread_join failed: %d\n", rc);
        exit(EXIT_FAILURE);
    }
}

/* Mutex initialization wrapper. */
static inline void Pthread_mutex_init(
    pthread_mutex_t *mutex,
    const pthread_mutexattr_t *attr)
{
    int rc = pthread_mutex_init(mutex, attr);

    if (rc != 0) {
        fprintf(stderr, "pthread_mutex_init failed: %d\n", rc);
        exit(EXIT_FAILURE);
    }
}

/* Mutex lock wrapper. */
static inline void Pthread_mutex_lock(pthread_mutex_t *mutex)
{
    int rc = pthread_mutex_lock(mutex);

    if (rc != 0) {
        fprintf(stderr, "pthread_mutex_lock failed: %d\n", rc);
        exit(EXIT_FAILURE);
    }
}

/* Mutex unlock wrapper. */
static inline void Pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    int rc = pthread_mutex_unlock(mutex);

    if (rc != 0) {
        fprintf(stderr, "pthread_mutex_unlock failed: %d\n", rc);
        exit(EXIT_FAILURE);
    }
}

#endif /* COMMON_H */
