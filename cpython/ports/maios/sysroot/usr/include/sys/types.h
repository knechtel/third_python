#ifndef MAIOS_SYS_TYPES_H
#define MAIOS_SYS_TYPES_H

#include <stddef.h>
#include <stdint.h>

typedef int32_t ssize_t;
typedef int32_t off_t;
typedef uint32_t ino_t;
typedef uint32_t dev_t;
typedef uint32_t nlink_t;
typedef uint32_t mode_t;
typedef uint32_t uid_t;
typedef uint32_t gid_t;
typedef int32_t pid_t;
typedef int32_t blksize_t;
typedef int32_t blkcnt_t;
typedef int32_t clockid_t;
#define SSIZE_MAX INT32_MAX

#endif
