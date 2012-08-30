#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>

#include "mocktime.h"

#include <set>
#include <map>

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
static pthread_mutex_t mocked_time_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t mocked_time_sleep_cv = PTHREAD_COND_INITIALIZER;

static int gettimeofday_mocked(struct timeval *tv, struct timezone *unused)
{
    if (tv) {
        pthread_mutex_lock(&mocked_time_lock);
        *tv = mocked_time;
        pthread_mutex_unlock(&mocked_time_lock);
        return 0;
    } else {
        errno = EFAULT;
        return -1;
    }
}

static int pthread_cond_timedwait_mocked(pthread_cond_t *cond, pthread_mutex_t *mutex, 
                                         const struct timespec *abstime);

static int usleep_mocked(useconds_t useconds)
{
    pthread_mutex_lock(&mocked_time_lock);

    struct timeval sleep_duration;
    sleep_duration.tv_sec = useconds / 1000000;
    sleep_duration.tv_usec = useconds - (sleep_duration.tv_sec * 1000000);

    struct timeval future;
    timeradd(&mocked_time, &sleep_duration, &future);
    struct timespec future_ts;
    future_ts.tv_sec = future.tv_sec;
    future_ts.tv_nsec = future.tv_usec * 1000;

    pthread_cond_timedwait_mocked(&mocked_time_sleep_cv, &mocked_time_lock,
                                  &future_ts);
    pthread_mutex_unlock(&mocked_time_lock);

    return 0;
}

#define timespec_cmp(tvp, uvp, cmp)                                         \
    (((tvp)->tv_sec == (uvp)->tv_sec) ?                                 \
     ((tvp)->tv_nsec cmp (uvp)->tv_nsec) :                              \
     ((tvp)->tv_sec cmp (uvp)->tv_sec))


struct waiting_thread {
    pthread_t tid;
    pthread_cond_t *cond;
    pthread_mutex_t *mutex;
    struct timespec abstime;
    bool actually_signalled;

    bool operator<(const waiting_thread& other) const {
        return (timespec_cmp(&abstime, &other.abstime, <));
    }

    waiting_thread(pthread_t tid_, pthread_cond_t *cond_, 
                   pthread_mutex_t *mutex_, const struct timespec *abstime_) 
        : tid(tid_), cond(cond_), mutex(mutex_), actually_signalled(false) {
        if (abstime_) {
            abstime = *abstime_;
        } else {
            abstime.tv_sec = 0;
            abstime.tv_nsec = 0;
        }
    }
};

static pthread_mutex_t threads_lock = PTHREAD_MUTEX_INITIALIZER;
typedef std::set<waiting_thread*> WaitingThreadsSet;
static WaitingThreadsSet waiting_threads;
static std::map<pthread_cond_t *, waiting_thread*> waiting_threads_by_cv;
static std::set<pthread_t> child_threads;
static pthread_t main_thread;

struct wrapper_args {
    void *(*thread_fn)(void*);
    void *arg;
};

static void *
MocktimeWrapperThreadFn(void *arg)
{
    pthread_t tid = pthread_self();
    struct wrapper_args *wrap_args = (struct wrapper_args *) arg;
    void *(*thread_fn)(void*) = wrap_args->thread_fn;
    void *thread_arg = wrap_args->arg;
    
    delete wrap_args;
    wrap_args = NULL;
    void *ret = thread_fn(thread_arg);
    
    pthread_mutex_lock(&threads_lock);
    child_threads.erase(tid);
    pthread_mutex_unlock(&threads_lock);
    return ret;
}

int pthread_create_mocked(pthread_t *tid, const pthread_attr_t *attr, 
                          void *(*thread_fn)(void*), void *arg)
{
    struct wrapper_args *wrap_args = new struct wrapper_args;
    wrap_args->thread_fn = thread_fn;
    wrap_args->arg = arg;
    
    pthread_mutex_lock(&threads_lock);
    if (child_threads.empty()) {
        main_thread = pthread_self();
    }
    
    pthread_t new_thread;
    pthread_create(&new_thread, attr, MocktimeWrapperThreadFn, wrap_args);
    child_threads.insert(new_thread);
    if (tid) {
        *tid = new_thread;
    }
    pthread_mutex_unlock(&threads_lock);
    return new_thread;
}

/* must be holding thread_lock. */
static bool all_other_threads_are_sleeping()
{
    pthread_t self = pthread_self();
    for (std::set<pthread_t>::const_iterator i = child_threads.begin();
         i != child_threads.end(); ++i) {
        pthread_t cur_tid = *i;
        if (cur_tid == self) {
            continue;
        }

        bool found_waiter = false;
        for (WaitingThreadsSet::iterator j = waiting_threads.begin();
             j != waiting_threads.end(); ++j) {
            const waiting_thread* waiter = *j;
            if (waiter->tid == cur_tid) {
                found_waiter = true;
                break;
            }
        }
        if (!found_waiter) {
            return false;
        }
    }
    return true;
}

static void get_time_possibly_locked(struct timeval *tv, pthread_mutex_t *mutex)
{
    assert(tv);
    if (mutex == &mocked_time_lock) {
        *tv = mocked_time;
    } else {
        mocktime_gettimeofday(tv, NULL);
    }
}

static void set_time_possibly_locked(const struct timeval *tv, pthread_mutex_t *mutex)
{
    assert(tv);
    if (mutex == &mocked_time_lock) {
        mocked_time = *tv;
    } else {
        mocktime_settimeofday(tv, NULL);
    }
}

/* mock-sleep this thread until abstime,
 * waking any threads that wanted to wake up earlier */
/* must be holding thread_lock. */
static void sleep_and_wake_others(const struct timespec *abstime,
                                  pthread_mutex_t *mutex)
{
    assert(abstime);
    
    struct timeval abstime_tv;
    abstime_tv.tv_sec = abstime->tv_sec;
    abstime_tv.tv_usec = abstime->tv_nsec / 1000;

    // sorted iteration by waiter.abstime
    for (WaitingThreadsSet::iterator it = waiting_threads.begin();
         it != waiting_threads.end(); ++it) {
        waiting_thread *waiter = *it;
        struct timeval waiter_abstime_tv;
        waiter_abstime_tv.tv_sec = waiter->abstime.tv_sec;
        waiter_abstime_tv.tv_usec = waiter->abstime.tv_nsec / 1000;
        if (timercmp(&waiter_abstime_tv, &abstime_tv, >)) {
            // all waiters are waiting longer than for this one thread
            break;
        } else {
            set_time_possibly_locked(&waiter_abstime_tv, mutex);
            pthread_mutex_lock(waiter->mutex);
            pthread_cond_broadcast(waiter->cond);
            pthread_mutex_unlock(waiter->mutex);
        }
    }
    set_time_possibly_locked(&abstime_tv, mutex);
}

int pthread_cond_timedwait_mocked(pthread_cond_t *cond, pthread_mutex_t *mutex, 
                                  const struct timespec *abstime)
{
    int rc = 0;

    struct timeval now, abstime_tv;
    get_time_possibly_locked(&now, mutex);
    
    assert(abstime);
    abstime_tv.tv_sec = abstime->tv_sec;
    abstime_tv.tv_usec = abstime->tv_nsec / 1000;
    
    pthread_mutex_lock(&threads_lock);
    if (all_other_threads_are_sleeping()) {
        // I'm the only runnable thread, so this is just a sleep.
        sleep_and_wake_others(abstime, mutex);
        rc = ETIMEDOUT;
    } else {
        struct waiting_thread waiter(pthread_self(), cond, mutex, abstime);
        waiting_threads.insert(&waiter);
        waiting_threads_by_cv[cond] = &waiter;
        
        pthread_mutex_unlock(&threads_lock);
        get_time_possibly_locked(&now, mutex);
        while (timercmp(&now, &abstime_tv, <)) {
            pthread_cond_wait(cond, mutex);
            if (waiter.actually_signalled) {
                break;
            }
            get_time_possibly_locked(&now, mutex);
        }
        if (!waiter.actually_signalled) {
            rc = ETIMEDOUT;
        }
        pthread_mutex_lock(&threads_lock);
        
        waiting_threads.erase(&waiter);
        waiting_threads_by_cv.erase(cond);
    }
    pthread_mutex_unlock(&threads_lock);
    return rc;
}

static void mark_cv_signalled(pthread_cond_t *cond)
{
    pthread_mutex_lock(&threads_lock);
    struct waiting_thread *waiter = waiting_threads_by_cv[cond];
    waiter->actually_signalled = true;
    pthread_mutex_unlock(&threads_lock);
}

int pthread_cond_signal_mocked(pthread_cond_t *cond)
{
    mark_cv_signalled(cond);
    return pthread_cond_signal(cond);
}

int pthread_cond_broadcast_mocked(pthread_cond_t *cond)
{
    mark_cv_signalled(cond);
    return pthread_cond_broadcast(cond);
}

static int (*gettimeofday_fn)(struct timeval *, struct timezone *) = gettimeofday;
static int (*usleep_fn)(useconds_t) = usleep;
static int (*pthread_create_fn)(pthread_t *, const pthread_attr_t *,
                                void *(*)(void*), void*) = pthread_create;
static int (*pthread_cond_timedwait_fn)(pthread_cond_t *, pthread_mutex_t *, 
                                        const struct timespec *) = pthread_cond_timedwait;
static int (*pthread_cond_signal_fn)(pthread_cond_t *) = pthread_cond_signal;
static int (*pthread_cond_broadcast_fn)(pthread_cond_t *) = pthread_cond_broadcast;

struct mocked_fn {
    void **fn_ptr;
    void *mocked_fn;
    void *real_fn;
};

static struct mocked_fn mocked_fns[] = {
    {(void **)&gettimeofday_fn, (void *) gettimeofday_mocked, (void *) gettimeofday},
    {(void **)&usleep_fn, (void *) usleep_mocked, (void *) usleep},
    {(void **)&pthread_create_fn, (void *) pthread_create_mocked, (void *) pthread_create},
    {(void **)&pthread_cond_timedwait_fn, (void *) pthread_cond_timedwait_mocked, (void *) pthread_cond_timedwait},
    {(void **)&pthread_cond_signal_fn, (void *) pthread_cond_signal_mocked, (void *) pthread_cond_signal},
    {(void **)&pthread_cond_broadcast_fn, (void *) pthread_cond_broadcast_mocked, (void *) pthread_cond_broadcast},
};
static const size_t NUM_FNS = sizeof(mocked_fns) / sizeof(struct mocked_fn);

int mocktime_gettimeofday(struct timeval *tv, struct timezone *unused)
{
    return gettimeofday_fn(tv, unused);
}

int mocktime_settimeofday(const struct timeval *tv, const struct timezone *unused)
{
    if (tv) {
        pthread_mutex_lock(&mocked_time_lock);
        mocked_time = *tv;
        pthread_mutex_unlock(&mocked_time_lock);
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

CDECL int mocktime_pthread_create(pthread_t *tid, const pthread_attr_t *attr, 
                                  void *(*thread_fn)(void*), void *arg)
{
    return pthread_create_fn(tid, attr, thread_fn, arg);
}

int mocktime_pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, 
                                    const struct timespec *abstime)
{
    return pthread_cond_timedwait_fn(cond, mutex, abstime);
}

int mocktime_pthread_cond_signal(pthread_cond_t *cond)
{
    return pthread_cond_signal_fn(cond);
}

int mocktime_pthread_cond_broadcast(pthread_cond_t *cond)
{
    return pthread_cond_broadcast_fn(cond);
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
