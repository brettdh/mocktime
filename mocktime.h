#ifndef MOCKTIME_H_IS_INCLUDED
#define MOCKTIME_H_IS_INCLUDED

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

void mocktime_enable_mocking();
void mocktime_disable_mocking();

int mocktime_gettimeofday(struct timeval *tv, struct timezone *unused);
int mocktime_settimeofday(const struct timeval *tv, const struct timezone *unused);

int mocktime_usleep(useconds_t useconds);

#endif
