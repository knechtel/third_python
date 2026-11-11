#ifndef MAIOS_WCHAR_H
#define MAIOS_WCHAR_H

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

typedef int32_t wint_t;
typedef struct { int __state; } mbstate_t;
struct tm;

#define WCHAR_MIN INT32_MIN
#define WCHAR_MAX INT32_MAX
#define WEOF ((wint_t)-1)

size_t wcslen(const wchar_t *text);
int wcscmp(const wchar_t *left, const wchar_t *right);
int wcsncmp(const wchar_t *left, const wchar_t *right, size_t count);
wchar_t *wcscpy(wchar_t *dest, const wchar_t *src);
wchar_t *wcsncpy(wchar_t *dest, const wchar_t *src, size_t count);
wchar_t *wcschr(const wchar_t *text, wchar_t character);
wchar_t *wcsrchr(const wchar_t *text, wchar_t character);
wchar_t *wcstok(wchar_t *text, const wchar_t *delim, wchar_t **context);
long wcstol(const wchar_t *text, wchar_t **end, int base);
size_t wcsftime(wchar_t *buffer, size_t max, const wchar_t *format,
                const struct tm *timep);
int mbsinit(const mbstate_t *state);
size_t mbrtowc(wchar_t *pwc, const char *s, size_t n, mbstate_t *state);
size_t wcrtomb(char *s, wchar_t wc, mbstate_t *state);
int vswprintf(wchar_t *buffer, size_t count, const wchar_t *format, va_list args);
int swprintf(wchar_t *buffer, size_t count, const wchar_t *format, ...);

#endif
