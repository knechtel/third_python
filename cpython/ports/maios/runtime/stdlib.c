#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <wchar.h>

static int digit_value(unsigned int c)
{
    if (c >= '0' && c <= '9') return (int)(c - '0');
    if (c >= 'a' && c <= 'z') return (int)(c - 'a') + 10;
    if (c >= 'A' && c <= 'Z') return (int)(c - 'A') + 10;
    return -1;
}

static unsigned long parse_unsigned(const char *text, char **end, int base,
                                    unsigned long limit, int *negative,
                                    int *did_overflow)
{
    const char *cursor = text;
    unsigned long value = 0;
    int overflow = 0;
    int digit;

    while (isspace((unsigned char)*cursor)) ++cursor;
    *negative = 0;
    *did_overflow = 0;
    if (*cursor == '+' || *cursor == '-') {
        *negative = *cursor == '-';
        ++cursor;
    }
    if (base != 0 && (base < 2 || base > 36)) {
        errno = EINVAL;
        if (end != NULL) *end = (char *)text;
        return 0;
    }
    if ((base == 0 || base == 16) && cursor[0] == '0' &&
        (cursor[1] == 'x' || cursor[1] == 'X') &&
        digit_value((unsigned char)cursor[2]) >= 0 &&
        digit_value((unsigned char)cursor[2]) < 16) {
        cursor += 2;
        base = 16;
    }
    if (base == 0) base = *cursor == '0' ? 8 : 10;

    const char *digits = cursor;
    while ((digit = digit_value((unsigned char)*cursor)) >= 0 && digit < base) {
        if (value > (limit - (unsigned long)digit) / (unsigned long)base) {
            overflow = 1;
            value = limit;
        } else if (!overflow) {
            value = value * (unsigned long)base + (unsigned long)digit;
        }
        ++cursor;
    }
    if (cursor == digits) {
        if (end != NULL) *end = (char *)text;
        return 0;
    }
    if (end != NULL) *end = (char *)cursor;
    if (overflow) errno = ERANGE;
    *did_overflow = overflow;
    return value;
}

unsigned long strtoul(const char *text, char **end, int base)
{
    int negative, overflow;
    unsigned long value = parse_unsigned(text, end, base, ULONG_MAX,
                                         &negative, &overflow);
    return negative ? 0UL - value : value;
}

long strtol(const char *text, char **end, int base)
{
    int negative, overflow;
    unsigned long limit = (unsigned long)LONG_MAX + 1UL;
    unsigned long value = parse_unsigned(text, end, base, limit,
                                         &negative, &overflow);
    if (overflow) return negative ? LONG_MIN : LONG_MAX;
    if (negative) {
        if (value > (unsigned long)LONG_MAX + 1UL) {
            errno = ERANGE;
            return LONG_MIN;
        }
        return value == (unsigned long)LONG_MAX + 1UL ? LONG_MIN : -(long)value;
    }
    if (value > (unsigned long)LONG_MAX) {
        errno = ERANGE;
        return LONG_MAX;
    }
    return (long)value;
}

long wcstol(const wchar_t *text, wchar_t **end, int base)
{
    const wchar_t *cursor = text;
    unsigned long value = 0;
    unsigned long limit;
    int negative = 0, overflow = 0, digit;

    while (*cursor == L' ' || (*cursor >= L'\t' && *cursor <= L'\r')) ++cursor;
    if (*cursor == L'+' || *cursor == L'-') {
        negative = *cursor == L'-';
        ++cursor;
    }
    if (base != 0 && (base < 2 || base > 36)) {
        errno = EINVAL;
        if (end != NULL) *end = (wchar_t *)text;
        return 0;
    }
    if ((base == 0 || base == 16) && cursor[0] == L'0' &&
        (cursor[1] == L'x' || cursor[1] == L'X') &&
        digit_value((unsigned int)cursor[2]) >= 0 &&
        digit_value((unsigned int)cursor[2]) < 16) {
        cursor += 2;
        base = 16;
    }
    if (base == 0) base = *cursor == L'0' ? 8 : 10;
    limit = (unsigned long)LONG_MAX + (negative ? 1UL : 0UL);
    const wchar_t *digits = cursor;
    while ((digit = digit_value((unsigned int)*cursor)) >= 0 && digit < base) {
        if (value > (limit - (unsigned long)digit) / (unsigned long)base) {
            overflow = 1;
            value = limit;
        } else if (!overflow) {
            value = value * (unsigned long)base + (unsigned long)digit;
        }
        ++cursor;
    }
    if (cursor == digits) {
        if (end != NULL) *end = (wchar_t *)text;
        return 0;
    }
    if (end != NULL) *end = (wchar_t *)cursor;
    if (overflow) {
        errno = ERANGE;
        return negative ? LONG_MIN : LONG_MAX;
    }
    if (negative)
        return value == (unsigned long)LONG_MAX + 1UL ? LONG_MIN : -(long)value;
    return (long)value;
}

char *getenv(const char *name)
{
    (void)name;
    return NULL;
}

int abs(int value) { return value < 0 ? -value : value; }
long labs(long value) { return value < 0 ? -value : value; }
int isfinite(double value) { return __builtin_isfinite(value); }
int isinf(double value) { return __builtin_isinf(value); }
int isnan(double value) { return __builtin_isnan(value); }
int signbit(double value) { return __builtin_signbit(value); }

char *strerror(int error)
{
    switch (error) {
        case 0: return "Success";
        case EACCES: return "Permission denied";
        case EBADF: return "Bad file descriptor";
        case EEXIST: return "File exists";
        case EINVAL: return "Invalid argument";
        case EIO: return "I/O error";
        case EISDIR: return "Is a directory";
        case EMFILE: return "Too many open files";
        case ENOENT: return "No such file or directory";
        case ENOMEM: return "Out of memory";
        case ENOSPC: return "No space left on device";
        case ENOSYS: return "Function not implemented";
        case ENOTDIR: return "Not a directory";
        case ERANGE: return "Result out of range";
        case EROFS: return "Read-only file system";
        default: return "Unknown error";
    }
}

void abort(void)
{
    __builtin_trap();
}

void exit(int status)
{
    (void)status;
    __builtin_trap();
}
