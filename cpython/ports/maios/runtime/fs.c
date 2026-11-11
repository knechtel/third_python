#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ext4.h"

#define MAIOS_FD_FIRST 3
#define MAIOS_FD_COUNT 8
#define MAIOS_BLOCK_SIZE 1024

extern int maios_python_read_char(void);
extern void maios_python_write_char(char value);

typedef struct {
    int active;
    int writing;
    uint32_t size;
    uint32_t offset;
    uint32_t cached_block;
    int cache_valid;
    uint8_t cache[MAIOS_BLOCK_SIZE];
    char name[64];
} maios_fd_t;

static maios_fd_t descriptors[MAIOS_FD_COUNT];
static int active_reader = -1;
static int active_writer = -1;
static uint32_t write_offset;

static int fd_index(int fd)
{
    int index = fd - MAIOS_FD_FIRST;
    return index >= 0 && index < MAIOS_FD_COUNT && descriptors[index].active
               ? index : -1;
}

static int valid_name(const char *path, char name[64])
{
    size_t length = 0;
    if (path == NULL) return 0;
    if (path[0] == '/') ++path;
    while (path[length] != '\0') {
        if (path[length] == '/' || path[length] == '\\' || length >= 63) return 0;
        name[length] = path[length];
        ++length;
    }
    if (length == 0) return 0;
    name[length] = '\0';
    return 1;
}

int open(const char *path, int flags, ...)
{
    char name[64];
    int index;
    int writing = (flags & O_ACCMODE) != O_RDONLY;

    if (!valid_name(path, name)) { errno = ENOENT; return -1; }
    if (writing ? active_writer >= 0 : active_reader >= 0) {
        errno = EBUSY;
        return -1;
    }
    for (index = 0; index < MAIOS_FD_COUNT && descriptors[index].active; ++index) { }
    if (index == MAIOS_FD_COUNT) { errno = EMFILE; return -1; }

    maios_fd_t *descriptor = &descriptors[index];
    memset(descriptor, 0, sizeof(*descriptor));
    if (writing) {
        if (!ext4_write_begin(name)) { errno = EIO; return -1; }
        active_writer = index;
        write_offset = 0;
        descriptor->writing = 1;
    } else {
        if (!ext4_read_start(name, &descriptor->size)) { errno = ENOENT; return -1; }
        active_reader = index;
    }
    strcpy(descriptor->name, name);
    descriptor->active = 1;
    return index + MAIOS_FD_FIRST;
}

int openat(int dirfd, const char *path, int flags, ...)
{
    if (dirfd != AT_FDCWD) { errno = ENOSYS; return -1; }
    return open(path, flags);
}

int close(int fd)
{
    int index = fd_index(fd);
    if (index < 0) {
        if (fd >= 0 && fd <= 2) return 0;
        errno = EBADF;
        return -1;
    }
    maios_fd_t *descriptor = &descriptors[index];
    if (descriptor->writing) {
        if (!ext4_write_finish(write_offset)) { errno = EIO; return -1; }
        active_writer = -1;
    } else {
        ext4_read_end();
        active_reader = -1;
    }
    memset(descriptor, 0, sizeof(*descriptor));
    return 0;
}

ssize_t read(int fd, void *buffer, size_t count)
{
    if (fd == 0) {
        uint8_t *out = buffer;
        size_t read_count = 0;
        while (read_count < count) {
            int key = maios_python_read_char();
            if (key == 4) break;
            out[read_count++] = (uint8_t)key;
            if (key == '\n' || key == '\r') break;
        }
        return (ssize_t)read_count;
    }
    int index = fd_index(fd);
    if (index < 0 || descriptors[index].writing) { errno = EBADF; return -1; }
    maios_fd_t *descriptor = &descriptors[index];
    if (descriptor->offset >= descriptor->size) return 0;
    if (count > descriptor->size - descriptor->offset)
        count = descriptor->size - descriptor->offset;

    uint8_t *out = buffer;
    size_t copied = 0;
    while (copied < count) {
        uint32_t block = descriptor->offset / MAIOS_BLOCK_SIZE;
        size_t block_offset = descriptor->offset % MAIOS_BLOCK_SIZE;
        size_t chunk = MAIOS_BLOCK_SIZE - block_offset;
        if (chunk > count - copied) chunk = count - copied;
        if (!descriptor->cache_valid || descriptor->cached_block != block) {
            if (!ext4_read_block(block, descriptor->cache)) {
                errno = EIO;
                return copied ? (ssize_t)copied : -1;
            }
            descriptor->cached_block = block;
            descriptor->cache_valid = 1;
        }
        memcpy(out + copied, descriptor->cache + block_offset, chunk);
        descriptor->offset += (uint32_t)chunk;
        copied += chunk;
    }
    return (ssize_t)copied;
}

ssize_t write(int fd, const void *buffer, size_t count)
{
    if (fd == 1 || fd == 2) {
        const char *text = buffer;
        for (size_t i = 0; i < count; ++i) maios_python_write_char(text[i]);
        return (ssize_t)count;
    }
    int index = fd_index(fd);
    if (index < 0 || !descriptors[index].writing) { errno = EBADF; return -1; }
    const uint8_t *input = buffer;
    size_t written = 0;
    while (written < count) {
        size_t chunk = count - written;
        if (chunk > MAIOS_BLOCK_SIZE) chunk = MAIOS_BLOCK_SIZE;
        if (!ext4_write_block(write_offset / MAIOS_BLOCK_SIZE,
                              input + written, chunk)) {
            errno = EIO;
            return written ? (ssize_t)written : -1;
        }
        write_offset += (uint32_t)chunk;
        written += chunk;
    }
    descriptors[index].size = write_offset;
    return (ssize_t)written;
}

off_t lseek(int fd, off_t offset, int whence)
{
    int index = fd_index(fd);
    if (index < 0 || descriptors[index].writing) { errno = ESPIPE; return -1; }
    maios_fd_t *descriptor = &descriptors[index];
    int64_t target = offset;
    if (whence == SEEK_CUR) target += descriptor->offset;
    else if (whence == SEEK_END) target += descriptor->size;
    else if (whence != SEEK_SET) { errno = EINVAL; return -1; }
    if (target < 0 || target > descriptor->size) { errno = EINVAL; return -1; }
    descriptor->offset = (uint32_t)target;
    return (off_t)descriptor->offset;
}

static void fill_stat(struct stat *result, mode_t type, uint32_t size)
{
    memset(result, 0, sizeof(*result));
    result->st_mode = type | 0444;
    result->st_nlink = 1;
    result->st_size = (off_t)size;
    result->st_blksize = MAIOS_BLOCK_SIZE;
    result->st_blocks = (blkcnt_t)((size + MAIOS_BLOCK_SIZE - 1) / MAIOS_BLOCK_SIZE);
}

int stat(const char *path, struct stat *result)
{
    char name[64];
    uint32_t size;
    if (!valid_name(path, name)) { errno = ENOENT; return -1; }
    if (active_reader >= 0) { errno = EBUSY; return -1; }
    if (ext4_read_start(name, &size)) {
        ext4_read_end();
        fill_stat(result, S_IFREG, size);
        return 0;
    }
    ext4_entry_t entries[64];
    size_t count = ext4_list(entries, 64);
    for (size_t i = 0; i < count; ++i) {
        if (strcmp(entries[i].name, name) == 0 && entries[i].directory) {
            fill_stat(result, S_IFDIR, 0);
            return 0;
        }
    }
    errno = ENOENT;
    return -1;
}

int fstat(int fd, struct stat *result)
{
    if (fd >= 0 && fd <= 2) {
        fill_stat(result, S_IFCHR, 0);
        return 0;
    }
    int index = fd_index(fd);
    if (index < 0) { errno = EBADF; return -1; }
    fill_stat(result, S_IFREG, descriptors[index].size);
    return 0;
}

int fstatat(int dirfd, const char *path, struct stat *result, int flags)
{
    (void)flags;
    if (dirfd != AT_FDCWD) { errno = ENOSYS; return -1; }
    return stat(path, result);
}

int lstat(const char *path, struct stat *result) { return stat(path, result); }

int mkdir(const char *path, mode_t mode)
{
    char name[64];
    (void)mode;
    if (!valid_name(path, name)) { errno = EINVAL; return -1; }
    if (!ext4_mkdir(name)) { errno = EIO; return -1; }
    return 0;
}
