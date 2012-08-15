#include "mocktime.h"
#include "ctest.h"

CTEST_DATA(test_gettimeofday) {};

CTEST_SETUP(test_gettimeofday)
{
    mocktime_enable_mocking();
}

CTEST2(test_gettimeofday, time_observed)
{
    struct timeval mock_now = {42, 42};
    mocktime_settimeofday(&mock_now, NULL);

    struct timeval test_time = {0, 0};
    mocktime_gettimeofday(&test_time, NULL);
    ASSERT_EQUAL(mock_now.tv_sec, test_time.tv_sec);
    ASSERT_EQUAL(mock_now.tv_usec, test_time.tv_usec);
}
