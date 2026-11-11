#ifndef MAIOS_TIME_H
#define MAIOS_TIME_H

#include <stddef.h>
#include <stdint.h>

typedef int32_t clock_t;
typedef int32_t time_t;
typedef int32_t clockid_t;

struct timespec {
    time_t tv_sec;
    long tv_nsec;
};
struct tm {
    int tm_sec, tm_min, tm_hour, tm_mday, tm_mon, tm_year;
    int tm_wday, tm_yday, tm_isdst;
};

#define CLOCKS_PER_SEC 1000
#define TIME_UTC 1
#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 1

time_t time(time_t *result);
clock_t clock(void);
struct tm *gmtime(const time_t *timep);
struct tm *localtime(const time_t *timep);
struct tm *gmtime_r(const time_t *timep, struct tm *result);
struct tm *localtime_r(const time_t *timep, struct tm *result);
int clock_gettime(clockid_t clock_id, struct timespec *result);
int clock_getres(clockid_t clock_id, struct timespec *result);
size_t strftime(char *buffer, size_t max, const char *format,
                const struct tm *timep);
int timespec_get(struct timespec *ts, int base);

#endif
