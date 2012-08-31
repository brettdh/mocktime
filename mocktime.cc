#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <assert.h>

#include "mocktime.h"

/* tvb, tve, tvr should be of type TIMEVAL */
/* tve should be >= tvb. */
#define TIMEDIFF(tvb,tve,tvr)                                           \
    do {                                                                \
        assert(((tve).tv_sec > (tvb).tv_sec)                            \
               || (((tve).tv_sec == (tvb).tv_sec)                       \
                   && ((tve).tv_usec >= (tvb).tv_usec)));               \
        if ((tve).tv_usec < (tvb).tv_usec) {                            \
            (tvr).tv_usec = 1000000 + (tve).tv_usec - (tvb).tv_usec;    \
            (tvr).tv_sec = (tve).tv_sec - (tvb).tv_sec - 1;             \
        } else {                                                        \
            (tvr).tv_usec = (tve).tv_usec - (tvb).tv_usec;              \
            (tvr).tv_sec = (tve).tv_sec - (tvb).tv_sec;                 \
        }                                                               \
    } while (0)


static struct timeval mocked_time;

static int gettimeofday_mocked(struct timeval *tv, void *unused)
{
    if (tv) {
        *tv = mocked_time;
        return 0;
    } else {
        errno = EFAULT;
        return -1;
    }
}

static int usleep_mocked(useconds_t useconds)
{
    struct timeval sleep_duration = {
        useconds / 1000000,
        useconds - ((useconds / 1000000) * 1000000)
    };
    timeradd(&mocked_time, &sleep_duration, &mocked_time);
    return 0;
}

static int (*gettimeofday_fn)(struct timeval *, void *) = 
    (int (*)(struct timeval *, void *)) gettimeofday;
static int (*usleep_fn)(useconds_t) = usleep;

struct mocked_fn {
    void **fn_ptr;
    void *mocked_fn;
    void *real_fn;
};

static struct mocked_fn mocked_fns[] = {
    {(void **)&gettimeofday_fn, (void *) gettimeofday_mocked, (void *) gettimeofday},
    {(void **)&usleep_fn, (void *) usleep_mocked, (void *) usleep},
};
static const size_t NUM_FNS = sizeof(mocked_fns) / sizeof(struct mocked_fn);

int mocktime_gettimeofday(struct timeval *tv, void *unused)
{
    return gettimeofday_fn(tv, unused);
}

int mocktime_settimeofday(const struct timeval *tv, const void *unused)
{
    if (tv) {
        mocked_time = *tv;
        return 0;
    } else {
        errno = EFAULT;
        return -1;
    }
}

int mocktime_usleep(useconds_t useconds)
{
    return usleep_fn(useconds);
}

void mocktime_enable_mocking()
{
    size_t i;
    for (i = 0; i < NUM_FNS; ++i) {
        *(mocked_fns[i].fn_ptr) = mocked_fns[i].mocked_fn;
    }
}

void mocktime_disable_mocking()
{
    size_t i;
    for (i = 0; i < NUM_FNS; ++i) {
        *(mocked_fns[i].fn_ptr) = mocked_fns[i].real_fn;
    }
}
