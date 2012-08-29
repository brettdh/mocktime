#ifndef TEST_COMMON_H_INCL
#define TEST_COMMON_H_INCL

#define ASSERT_TIMEVAL_EQUAL(expected, actual)                      \
    do {                                                            \
        ASSERT_EQUAL((expected).tv_sec, (actual).tv_sec);           \
        ASSERT_EQUAL((expected).tv_usec, (actual).tv_usec);         \
    } while (0)

#endif
