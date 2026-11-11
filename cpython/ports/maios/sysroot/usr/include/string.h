#ifndef MAIOS_STRING_H
#define MAIOS_STRING_H

#include <stddef.h>

void *memcpy(void *dest, const void *src, size_t count);
void *memmove(void *dest, const void *src, size_t count);
void *memset(void *dest, int value, size_t count);
int memcmp(const void *left, const void *right, size_t count);
size_t strlen(const char *text);
size_t strnlen(const char *text, size_t count);
int strcmp(const char *left, const char *right);
int strncmp(const char *left, const char *right, size_t count);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t count);
char *strcat(char *dest, const char *src);
char *strchr(const char *text, int character);
char *strrchr(const char *text, int character);
char *strstr(const char *text, const char *pattern);
size_t strcspn(const char *text, const char *reject);
size_t strspn(const char *text, const char *accept);
void *memchr(const void *memory, int value, size_t count);
char *strerror(int error);
char *strdup(const char *text);

#endif
