#ifndef MOCKTIME_H_IS_INCLUDED
#define MOCKTIME_H_IS_INCLUDED

#include <sys/time.h>

int mocktime_gettimeofday(struct timeval *tv, void *unused);
int mocktime_settimeofday(const struct timeval *tv, void *unused);

#endif
