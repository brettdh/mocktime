#include "mocktime.h"
#include "ctest.h"

#define ASSERT_TIMEVAL_EQUAL(expected, actual)                      \
    do {                                                            \
        ASSERT_EQUAL((expected).tv_sec, (actual).tv_sec);           \
        ASSERT_EQUAL((expected).tv_usec, (actual).tv_usec);         \
    } while (0)

CTEST_DATA(test_sleep) {};

CTEST_SETUP(test_sleep)
{
    mocktime_enable_mocking();
}

CTEST2(test_sleep, later_time_observed)
{
    struct timeval mock_now = {42, 42};
    mocktime_settimeofday(&mock_now, NULL);

    struct timeval test_time = {0, 0};
    mocktime_gettimeofday(&test_time, NULL);
    ASSERT_TIMEVAL_EQUAL(mock_now, test_time);

    mocktime_usleep(1000000);
    mocktime_gettimeofday(&test_time, NULL);
    mock_now.tv_sec = 43;
    ASSERT_TIMEVAL_EQUAL(mock_now, test_time);
}
