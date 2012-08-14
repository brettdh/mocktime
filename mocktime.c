#include <errno.h>

#include "mocktime.h"

static struct timeval mocked_time;

int mocktime_gettimeofday(struct timeval *tv, void *unused)
{
    if (tv) {
        *tv = mocked_time;
        return 0;
    } else {
        errno = EFAULT;
        return -1;
    }
}

int mocktime_settimeofday(const struct timeval *tv, void *unused)
{
    if (tv) {
        mocked_time = *tv;
        return 0;
    } else {
        errno = EFAULT;
        return -1;
    }
}

