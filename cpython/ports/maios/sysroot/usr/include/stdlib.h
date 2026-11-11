#ifndef MAIOS_STDLIB_H
#define MAIOS_STDLIB_H

#include <stddef.h>
#include <stdint.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

void *malloc(size_t size);
void *calloc(size_t count, size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);
void abort(void) __attribute__((noreturn));
void exit(int status) __attribute__((noreturn));
char *getenv(const char *name);
int setenv(const char *name, const char *value, int overwrite);
int unsetenv(const char *name);
long strtol(const char *text, char **end, int base);
unsigned long strtoul(const char *text, char **end, int base);
long long strtoll(const char *text, char **end, int base);
unsigned long long strtoull(const char *text, char **end, int base);
double strtod(const char *text, char **end);
size_t mbstowcs(wchar_t *dest, const char *src, size_t count);
size_t wcstombs(char *dest, const wchar_t *src, size_t count);
int atoi(const char *text);
long atol(const char *text);
int abs(int value);
long labs(long value);
void qsort(void *base, size_t count, size_t size,
           int (*compare)(const void *, const void *));

#endif
