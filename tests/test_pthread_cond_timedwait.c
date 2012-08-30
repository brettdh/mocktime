#include "mocktime.h"
#include "ctest.h"
#include "test_common.h"

#include <pthread.h>
#include <errno.h>

CTEST_DATA(test_pthread_cond_timedwait) {};

CTEST_SETUP(test_pthread_cond_timedwait)
{
    mocktime_enable_mocking();
}

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
/*static pthread_cond_t finish_cv = PTHREAD_COND_INITIALIZER;*/

CTEST2(test_pthread_cond_timedwait, later_time_observed)
{
    struct timeval mock_now = {42, 42};
    mocktime_settimeofday(&mock_now, NULL);

    struct timeval test_time;
    mocktime_gettimeofday(&test_time, NULL);
    ASSERT_TIMEVAL_EQUAL(mock_now, test_time);

    struct timespec abstime = {43, 0};
    pthread_mutex_lock(&mutex);
    int rc = mocktime_pthread_cond_timedwait(&cv, &mutex, &abstime);
    pthread_mutex_unlock(&mutex);
    mocktime_gettimeofday(&test_time, NULL);
    mock_now.tv_sec = 43;
    mock_now.tv_usec = 0;
    ASSERT_TIMEVAL_EQUAL(mock_now, test_time);

    ASSERT_EQUAL(ETIMEDOUT, rc);
}

static void *
ChildFn(void *arg)
{
    pthread_mutex_lock(&mutex);
    mocktime_usleep(1);
    mocktime_pthread_cond_signal(&cv);
    pthread_mutex_unlock(&mutex);
    return NULL;
}

CTEST2(test_pthread_cond_timedwait, signal_observed)
{
    pthread_t child;
    
    struct timeval mock_now;
    struct timeval test_time;
    struct timespec abstime;

    gettimeofday(&mock_now, NULL);
    mocktime_settimeofday(&mock_now, NULL);

    abstime.tv_sec = mock_now.tv_sec + 1;
    abstime.tv_nsec = mock_now.tv_usec * 1000;
    pthread_mutex_lock(&mutex);
    mocktime_pthread_create(&child, NULL, ChildFn, NULL);
    int rc = mocktime_pthread_cond_timedwait(&cv, &mutex, &abstime);
    ASSERT_TRUE(rc != ETIMEDOUT);
    mocktime_gettimeofday(&test_time, NULL);
    pthread_mutex_unlock(&mutex);
    pthread_join(child, NULL);

    ++mock_now.tv_usec;
    ASSERT_TIMEVAL_EQUAL(mock_now, test_time);
}

void *
WaiterThreadFn(void *arg)
{
    struct timeval now, test_time;
    struct timespec abstime;
    mocktime_gettimeofday(&now, NULL);
    abstime.tv_sec = now.tv_sec + 2;
    abstime.tv_nsec = 0;
    
    int *done = (int *) arg;
    int rc;
    pthread_mutex_lock(&mutex);
    while (!*done) {
        rc = mocktime_pthread_cond_timedwait(&cv, &mutex, &abstime);
    }
    pthread_mutex_unlock(&mutex);
    
    mocktime_gettimeofday(&test_time, NULL);
    if ((abstime.tv_sec - test_time.tv_sec) > 1) {
        // return error if thread was not signalled
        //  (i.e. if it timed out)
        return (void *) -1;
    }
    
    return (void *) 0;
}

/* revisit this one.  it needs some careful attention to make work, but 
 * I might not need to use the library in this way. */
CTEST2_SKIP(test_pthread_cond_timedwait, signal_observed_multiple_threads)
{
    const size_t NUM_THREADS = 5;
    pthread_t threads[NUM_THREADS];

    struct timeval now;
    gettimeofday(&now, NULL);
    mocktime_settimeofday(&now, NULL);

    int done = 0;
    size_t i;

    pthread_mutex_lock(&mutex);
    for (i = 0; i < NUM_THREADS; ++i) {
        mocktime_pthread_create(&threads[i], NULL, WaiterThreadFn, &done);
    }
    pthread_mutex_unlock(&mutex);

    // all threads should sleep for a bit, but
    //  only this one should wake up again
    mocktime_usleep(1000);

    pthread_mutex_lock(&mutex);
    done = 1;
    mocktime_pthread_cond_broadcast(&cv);
    pthread_mutex_unlock(&mutex);

    int success = 1;
    int ret = 0;
    for (i = 0; i < NUM_THREADS; ++i) {
        pthread_join(threads[i], (void **)&ret);
        if (ret != 0) {
            success = 0;
        }
    }
    ASSERT_TRUE(success);
}
