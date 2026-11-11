#include <ctype.h>
#include <limits.h>
#include <locale.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>

static struct lconv maios_locale = {
    .decimal_point = ".",
    .thousands_sep = "",
    .grouping = "",
    .int_curr_symbol = "",
    .currency_symbol = "",
    .mon_decimal_point = "",
    .mon_thousands_sep = "",
    .mon_grouping = "",
    .positive_sign = "",
    .negative_sign = "",
    .int_frac_digits = CHAR_MAX,
    .frac_digits = CHAR_MAX,
    .p_cs_precedes = CHAR_MAX,
    .p_sep_by_space = CHAR_MAX,
    .n_cs_precedes = CHAR_MAX,
    .n_sep_by_space = CHAR_MAX,
    .p_sign_posn = CHAR_MAX,
    .n_sign_posn = CHAR_MAX,
};

int isalnum(int c) { return isalpha(c) || isdigit(c); }
int isalpha(int c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); }
int iscntrl(int c) { return (c >= 0 && c < 32) || c == 127; }
int isdigit(int c) { return c >= '0' && c <= '9'; }
int isgraph(int c) { return c > 32 && c < 127; }
int islower(int c) { return c >= 'a' && c <= 'z'; }
int isprint(int c) { return c >= 32 && c < 127; }
int ispunct(int c) { return isgraph(c) && !isalnum(c); }
int isspace(int c) { return c == ' ' || (c >= '\t' && c <= '\r'); }
int isupper(int c) { return c >= 'A' && c <= 'Z'; }
int isxdigit(int c)
{
    return isdigit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}
int tolower(int c) { return isupper(c) ? c + ('a' - 'A') : c; }
int toupper(int c) { return islower(c) ? c - ('a' - 'A') : c; }

char *setlocale(int category, const char *locale)
{
    (void)category;
    if (locale != NULL && locale[0] != '\0' &&
        !(locale[0] == 'C' && locale[1] == '\0') &&
        !(locale[0] == 'P' && locale[1] == 'O' && locale[2] == 'S' &&
          locale[3] == 'I' && locale[4] == 'X' && locale[5] == '\0')) {
        return NULL;
    }
    return "C";
}

struct lconv *localeconv(void) { return &maios_locale; }

size_t wcslen(const wchar_t *text)
{
    size_t length = 0;
    while (text[length] != 0) ++length;
    return length;
}

int wcscmp(const wchar_t *a, const wchar_t *b)
{
    while (*a && *a == *b) { ++a; ++b; }
    return *a < *b ? -1 : *a != *b;
}

int wcsncmp(const wchar_t *a, const wchar_t *b, size_t n)
{
    while (n && *a && *a == *b) { ++a; ++b; --n; }
    if (!n) return 0;
    return *a < *b ? -1 : *a != *b;
}

wchar_t *wcscpy(wchar_t *dest, const wchar_t *src)
{
    wchar_t *result = dest;
    while ((*dest++ = *src++) != 0) { }
    return result;
}

wchar_t *wcsncpy(wchar_t *dest, const wchar_t *src, size_t n)
{
    size_t i = 0;
    for (; i < n && src[i]; ++i) dest[i] = src[i];
    for (; i < n; ++i) dest[i] = 0;
    return dest;
}

wchar_t *wcschr(const wchar_t *text, wchar_t c)
{
    do { if (*text == c) return (wchar_t *)text; } while (*text++);
    return NULL;
}

wchar_t *wcsrchr(const wchar_t *text, wchar_t c)
{
    const wchar_t *found = NULL;
    do { if (*text == c) found = text; } while (*text++);
    return (wchar_t *)found;
}

wchar_t *wcstok(wchar_t *text, const wchar_t *delim, wchar_t **context)
{
    wchar_t *start = text != NULL ? text : *context;
    while (*start && wcschr(delim, *start)) ++start;
    if (!*start) { *context = start; return NULL; }
    wchar_t *end = start;
    while (*end && !wcschr(delim, *end)) ++end;
    if (*end) *end++ = 0;
    *context = end;
    return start;
}

size_t wcstombs(char *dest, const wchar_t *src, size_t n)
{
    size_t written = 0;
    while (*src) {
        uint32_t c = (uint32_t)*src++;
        unsigned char encoded[4];
        size_t count;
        if (c <= 0x7f) { encoded[0] = c; count = 1; }
        else if (c <= 0x7ff) {
            encoded[0] = 0xc0 | (c >> 6); encoded[1] = 0x80 | (c & 0x3f); count = 2;
        } else if (c <= 0xffff && (c < 0xd800 || c > 0xdfff)) {
            encoded[0] = 0xe0 | (c >> 12); encoded[1] = 0x80 | ((c >> 6) & 0x3f);
            encoded[2] = 0x80 | (c & 0x3f); count = 3;
        } else if (c <= 0x10ffff) {
            encoded[0] = 0xf0 | (c >> 18); encoded[1] = 0x80 | ((c >> 12) & 0x3f);
            encoded[2] = 0x80 | ((c >> 6) & 0x3f); encoded[3] = 0x80 | (c & 0x3f); count = 4;
        } else return (size_t)-1;
        if (dest != NULL) {
            if (count > n - written) return written;
            for (size_t i = 0; i < count; ++i) dest[written++] = (char)encoded[i];
        } else written += count;
    }
    return written;
}

size_t strcspn(const char *text, const char *reject)
{
    size_t length = 0;
    while (text[length] && strchr(reject, text[length]) == NULL) ++length;
    return length;
}
