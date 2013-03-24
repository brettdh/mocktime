#ifndef MOCKTIME_H_IS_INCLUDED
#define MOCKTIME_H_IS_INCLUDED

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#ifdef __cplusplus
#define CDECL extern "C"
#else
#define CDECL
#endif

CDECL void mocktime_enable_mocking();
CDECL void mocktime_disable_mocking();

CDECL int mocktime_gettimeofday(struct timeval *tv, void *unused);
CDECL int mocktime_settimeofday(const struct timeval *tv, const void *unused);

CDECL int mocktime_usleep(suseconds_t useconds);

#endif
