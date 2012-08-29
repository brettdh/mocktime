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
