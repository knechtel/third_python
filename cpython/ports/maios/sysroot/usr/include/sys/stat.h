#ifndef MAIOS_SYS_STAT_H
#define MAIOS_SYS_STAT_H

#include <sys/types.h>

struct timespec;
struct stat {
    dev_t st_dev;
    ino_t st_ino;
    mode_t st_mode;
    nlink_t st_nlink;
    uid_t st_uid;
    gid_t st_gid;
    dev_t st_rdev;
    off_t st_size;
    blksize_t st_blksize;
    blkcnt_t st_blocks;
    long st_atime;
    long st_mtime;
    long st_ctime;
};

#define S_IFMT 0170000
#define S_IFIFO 0010000
#define S_IFCHR 0020000
#define S_IFBLK 0060000
#define S_IFDIR 0040000
#define S_IFREG 0100000
#define S_IFLNK 0120000
#define S_IFSOCK 0140000
#define S_ISFIFO(mode) (((mode) & S_IFMT) == S_IFIFO)
#define S_ISCHR(mode) (((mode) & S_IFMT) == S_IFCHR)
#define S_ISBLK(mode) (((mode) & S_IFMT) == S_IFBLK)
#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#define S_ISREG(mode) (((mode) & S_IFMT) == S_IFREG)
#define S_ISLNK(mode) (((mode) & S_IFMT) == S_IFLNK)

int stat(const char *path, struct stat *result);
int fstat(int fd, struct stat *result);
int lstat(const char *path, struct stat *result);
int fstatat(int dirfd, const char *path, struct stat *result, int flags);
int mkdir(const char *path, mode_t mode);

#endif
