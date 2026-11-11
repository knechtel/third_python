#ifndef MAIOS_SYS_RANDOM_H
#define MAIOS_SYS_RANDOM_H

#include <stddef.h>
#include <sys/types.h>

ssize_t getrandom(void *buffer, size_t length, unsigned int flags);

#define GRND_NONBLOCK 0x0001
#define GRND_RANDOM 0x0002

#endif
