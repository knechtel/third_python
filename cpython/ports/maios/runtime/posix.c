#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stddef.h>
#include <unistd.h>

char *getcwd(char *buffer, size_t size)
{
    if (buffer == NULL || size < 2) {
        errno = ERANGE;
        return NULL;
    }
    buffer[0] = '/';
    buffer[1] = '\0';
    return buffer;
}

int isatty(int fd)
{
    if (fd >= STDIN_FILENO && fd <= STDERR_FILENO) {
        return 1;
    }
    errno = EBADF;
    return 0;
}

int dup(int fd)
{
    (void)fd;
    errno = ENOSYS;
    return -1;
}

int fcntl(int fd, int command, ...)
{
    (void)fd;
    switch (command) {
        case F_GETFD:
        case F_GETFL:
            return 0;
        case F_SETFD:
        case F_SETFL:
            return 0;
        default:
            errno = ENOSYS;
            return -1;
    }
}

int setenv(const char *name, const char *value, int overwrite)
{
    (void)name;
    (void)value;
    (void)overwrite;
    errno = ENOSYS;
    return -1;
}

int unsetenv(const char *name)
{
    (void)name;
    errno = ENOSYS;
    return -1;
}

sighandler_t signal(int sig, sighandler_t handler)
{
    (void)sig;
    (void)handler;
    return SIG_DFL;
}

int raise(int sig)
{
    (void)sig;
    errno = ENOSYS;
    return -1;
}
