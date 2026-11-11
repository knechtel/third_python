#include <errno.h>
#include <time.h>
#include <sys/time.h>

/* The kernel has no wall clock or monotonic timer interface yet. These
 * symbols let CPython link while reporting that limitation to callers. */
int clock_gettime(clockid_t clock_id, struct timespec *result)
{
    (void)clock_id;
    (void)result;
    errno = ENOSYS;
    return -1;
}

int clock_getres(clockid_t clock_id, struct timespec *result)
{
    (void)clock_id;
    (void)result;
    errno = ENOSYS;
    return -1;
}

int gettimeofday(struct timeval *result, void *timezone)
{
    (void)result;
    (void)timezone;
    errno = ENOSYS;
    return -1;
}

struct tm *gmtime_r(const time_t *timep, struct tm *result)
{
    (void)timep;
    (void)result;
    errno = ENOSYS;
    return 0;
}

struct tm *localtime_r(const time_t *timep, struct tm *result)
{
    (void)timep;
    (void)result;
    errno = ENOSYS;
    return 0;
}
