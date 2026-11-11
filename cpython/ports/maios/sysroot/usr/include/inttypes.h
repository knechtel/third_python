#ifndef MAIOS_INTTYPES_H
#define MAIOS_INTTYPES_H

#include <stdint.h>
#include <stddef.h>

typedef struct { intmax_t quot; intmax_t rem; } imaxdiv_t;
intmax_t strtoimax(const char *text, char **end, int base);
uintmax_t strtoumax(const char *text, char **end, int base);
imaxdiv_t imaxdiv(intmax_t numer, intmax_t denom);

#define PRId8 "d"
#define PRIi8 "i"
#define PRIo8 "o"
#define PRIu8 "u"
#define PRIx8 "x"
#define PRIX8 "X"
#define PRId16 "d"
#define PRIi16 "i"
#define PRIo16 "o"
#define PRIu16 "u"
#define PRIx16 "x"
#define PRIX16 "X"
#define PRId32 "d"
#define PRIi32 "i"
#define PRIo32 "o"
#define PRIu32 "u"
#define PRIx32 "x"
#define PRIX32 "X"
#define PRId64 "lld"
#define PRIi64 "lli"
#define PRIo64 "llo"
#define PRIu64 "llu"
#define PRIx64 "llx"
#define PRIX64 "llX"
#define PRIdPTR "d"
#define PRIuPTR "u"
#define PRIxPTR "x"
#define SCNd64 "lld"
#define SCNu64 "llu"
#define SCNx64 "llx"

#endif
