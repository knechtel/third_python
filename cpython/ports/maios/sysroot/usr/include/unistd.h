#ifndef MAIOS_UNISTD_H
#define MAIOS_UNISTD_H

#include <sys/types.h>

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

ssize_t read(int fd, void *buffer, size_t count);
ssize_t write(int fd, const void *buffer, size_t count);
int close(int fd);
int dup(int fd);
off_t lseek(int fd, off_t offset, int whence);
int unlink(const char *path);
int isatty(int fd);
int getpagesize(void);
char *getcwd(char *buffer, size_t size);

#endif
