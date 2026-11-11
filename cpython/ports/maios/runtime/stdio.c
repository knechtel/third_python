#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct maios_FILE {
    int fd;
    int owned;
    int eof;
    int error;
    int has_pushback;
    unsigned char pushback;
};

static struct maios_FILE input_stream = {0, 0, 0, 0, 0, 0};
static struct maios_FILE output_stream = {1, 0, 0, 0, 0, 0};
static struct maios_FILE error_stream = {2, 0, 0, 0, 0, 0};
FILE *stdin = &input_stream;
FILE *stdout = &output_stream;
FILE *stderr = &error_stream;

static FILE *new_stream(int fd, int owned)
{
    FILE *stream = malloc(sizeof(*stream));
    if (stream == NULL) return NULL;
    memset(stream, 0, sizeof(*stream));
    stream->fd = fd;
    stream->owned = owned;
    return stream;
}

FILE *fdopen(int fd, const char *mode)
{
    (void)mode;
    return new_stream(fd, 0);
}

FILE *fopen(const char *path, const char *mode)
{
    int flags = O_RDONLY;
    if (mode == NULL || mode[0] == '\0') { errno = EINVAL; return NULL; }
    if (mode[0] == 'w') flags = O_WRONLY | O_CREAT | O_TRUNC;
    else if (mode[0] == 'a') flags = O_WRONLY | O_CREAT | O_APPEND;
    int fd = open(path, flags, 0666);
    if (fd < 0) return NULL;
    FILE *stream = new_stream(fd, 1);
    if (stream == NULL) { close(fd); return NULL; }
    return stream;
}

int fileno(FILE *stream) { return stream == NULL ? -1 : stream->fd; }
int fflush(FILE *stream) { (void)stream; return 0; }
int fclose(FILE *stream)
{
    if (stream == NULL) { errno = EINVAL; return EOF; }
    int result = stream->owned ? close(stream->fd) : 0;
    if (stream->owned) free(stream);
    return result < 0 ? EOF : 0;
}
int fseek(FILE *stream, long offset, int whence)
{
    if (stream == NULL || lseek(stream->fd, (off_t)offset, whence) < 0) return -1;
    stream->eof = 0;
    stream->has_pushback = 0;
    return 0;
}
long ftell(FILE *stream)
{
    if (stream == NULL) return -1;
    return (long)lseek(stream->fd, 0, SEEK_CUR);
}
void rewind(FILE *stream) { (void)fseek(stream, 0, SEEK_SET); clearerr(stream); }
void clearerr(FILE *stream) { if (stream != NULL) { stream->eof = 0; stream->error = 0; } }
int feof(FILE *stream) { return stream != NULL && stream->eof; }
int ferror(FILE *stream) { return stream != NULL && stream->error; }

int fgetc(FILE *stream)
{
    unsigned char value;
    if (stream == NULL) return EOF;
    if (stream->has_pushback) { stream->has_pushback = 0; return stream->pushback; }
    ssize_t count = read(stream->fd, &value, 1);
    if (count == 1) return value;
    if (count == 0) stream->eof = 1;
    else stream->error = 1;
    return EOF;
}
int getc(FILE *stream) { return fgetc(stream); }
int ungetc(int character, FILE *stream)
{
    if (stream == NULL || character == EOF || stream->has_pushback) return EOF;
    stream->pushback = (unsigned char)character;
    stream->has_pushback = 1;
    stream->eof = 0;
    return stream->pushback;
}
int fputc(int character, FILE *stream)
{
    unsigned char value = (unsigned char)character;
    if (stream == NULL || write(stream->fd, &value, 1) != 1) {
        if (stream != NULL) stream->error = 1;
        return EOF;
    }
    return value;
}
int putchar(int character) { return fputc(character, stdout); }
int fputs(const char *text, FILE *stream)
{
    size_t length = strlen(text);
    ssize_t count = stream == NULL ? -1 : write(stream->fd, text, length);
    if (count < 0 || (size_t)count != length) {
        if (stream != NULL) stream->error = 1;
        return EOF;
    }
    return 0;
}
int puts(const char *text) { return fputs(text, stdout) < 0 || putchar('\n') == EOF ? EOF : 0; }
char *fgets(char *buffer, int size, FILE *stream)
{
    if (buffer == NULL || size <= 0 || stream == NULL) return NULL;
    int i = 0;
    while (i + 1 < size) {
        int c = fgetc(stream);
        if (c == EOF) break;
        buffer[i++] = (char)c;
        if (c == '\n') break;
    }
    if (i == 0) return NULL;
    buffer[i] = '\0';
    return buffer;
}
size_t fread(void *buffer, size_t size, size_t count, FILE *stream)
{
    if (size == 0 || count == 0 || stream == NULL) return 0;
    size_t total = size * count;
    size_t done = 0;
    unsigned char *out = buffer;
    while (done < total) {
        int c = fgetc(stream);
        if (c == EOF) break;
        out[done++] = (unsigned char)c;
    }
    return done / size;
}
size_t fwrite(const void *buffer, size_t size, size_t count, FILE *stream)
{
    if (size == 0 || count == 0 || stream == NULL) return 0;
    size_t total = size * count;
    ssize_t written = write(stream->fd, buffer, total);
    if (written < 0) { stream->error = 1; return 0; }
    return (size_t)written / size;
}
int setvbuf(FILE *stream, char *buffer, int mode, size_t size)
{
    (void)buffer; (void)mode; (void)size;
    return stream == NULL ? -1 : 0;
}
void setbuf(FILE *stream, char *buffer) { (void)setvbuf(stream, buffer, _IONBF, 0); }

typedef struct { char *buffer; size_t capacity; size_t length; } output_t;
static void emit(output_t *out, char c)
{
    if (out->capacity != 0 && out->length + 1 < out->capacity)
        out->buffer[out->length] = c;
    if (out->length < (size_t)INT32_MAX) ++out->length;
}
static void emit_repeat(output_t *out, char c, int count)
{
    while (count-- > 0) emit(out, c);
}
static void emit_unsigned(output_t *out, uintmax_t value, unsigned base,
                          int upper, int width, int precision, int left,
                          int zero, int alternate, int negative, int pointer)
{
    char digits[sizeof(value) * 8 + 1];
    const char *alphabet = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    int n = 0;
    do { digits[n++] = alphabet[value % base]; value /= base; } while (value != 0);
    while (precision > n) digits[n++] = '0';
    int prefix = pointer || (alternate && base == 16 && digits[n - 1] != '0');
    int sign = negative ? 1 : 0;
    int padding = width - n - prefix * 2 - sign;
    if (!left && !(zero && precision < 0)) emit_repeat(out, ' ', padding);
    if (negative) emit(out, '-');
    if (prefix) { emit(out, '0'); emit(out, upper ? 'X' : 'x'); }
    if (!left && zero && precision < 0) emit_repeat(out, '0', padding);
    while (n > 0) emit(out, digits[--n]);
    if (left) emit_repeat(out, ' ', padding);
}

static int format_to(output_t *out, const char *format, va_list args)
{
    while (*format) {
        if (*format != '%') { emit(out, *format++); continue; }
        ++format;
        if (*format == '%') { emit(out, *format++); continue; }
        int left = 0, plus = 0, space = 0, alternate = 0, zero = 0;
        for (;;) {
            if (*format == '-') left = 1;
            else if (*format == '+') plus = 1;
            else if (*format == ' ') space = 1;
            else if (*format == '#') alternate = 1;
            else if (*format == '0') zero = 1;
            else break;
            ++format;
        }
        int width = 0, precision = -1;
        if (*format == '*') { width = va_arg(args, int); ++format; if (width < 0) { left = 1; width = -width; } }
        else while (*format >= '0' && *format <= '9') width = width * 10 + (*format++ - '0');
        if (*format == '.') {
            ++format; precision = 0;
            if (*format == '*') { precision = va_arg(args, int); ++format; if (precision < 0) precision = -1; }
            else while (*format >= '0' && *format <= '9') precision = precision * 10 + (*format++ - '0');
        }
        enum { LEN_INT, LEN_LONG, LEN_LLONG, LEN_SIZE, LEN_PTRDIFF } length = LEN_INT;
        if (*format == 'h') { ++format; if (*format == 'h') ++format; }
        else if (*format == 'l') { ++format; length = LEN_LONG; if (*format == 'l') { ++format; length = LEN_LLONG; } }
        else if (*format == 'z' || *format == 'j') { ++format; length = LEN_SIZE; }
        else if (*format == 't') { ++format; length = LEN_PTRDIFF; }
        else if (*format == 'L') { ++format; }
        char conversion = *format ? *format++ : '\0';
        if (conversion == 's') {
            const char *text = va_arg(args, const char *);
            if (text == NULL) text = "(null)";
            size_t len = strlen(text);
            if (precision >= 0 && len > (size_t)precision) len = (size_t)precision;
            if (!left) emit_repeat(out, ' ', width - (int)len);
            for (size_t i = 0; i < len; ++i) emit(out, text[i]);
            if (left) emit_repeat(out, ' ', width - (int)len);
        } else if (conversion == 'c') {
            int c = va_arg(args, int);
            if (!left) emit_repeat(out, ' ', width - 1);
            emit(out, (char)c);
            if (left) emit_repeat(out, ' ', width - 1);
        } else if (conversion == 'd' || conversion == 'i') {
            intmax_t value;
            if (length == LEN_LLONG) value = va_arg(args, long long);
            else if (length == LEN_LONG) value = va_arg(args, long);
            else if (length == LEN_SIZE) value = va_arg(args, ptrdiff_t);
            else value = va_arg(args, int);
            int negative = value < 0;
            uintmax_t magnitude = negative ? (uintmax_t)(-(value + 1)) + 1 : (uintmax_t)value;
            if (!negative && (plus || space)) emit(out, plus ? '+' : ' ');
            emit_unsigned(out, magnitude, 10, 0, width, precision, left, zero, 0, negative, 0);
        } else if (conversion == 'u' || conversion == 'o' || conversion == 'x' || conversion == 'X' || conversion == 'p') {
            uintmax_t value;
            if (conversion == 'p') value = (uintptr_t)va_arg(args, void *);
            else if (length == LEN_LLONG) value = va_arg(args, unsigned long long);
            else if (length == LEN_LONG) value = va_arg(args, unsigned long);
            else if (length == LEN_SIZE) value = va_arg(args, size_t);
            else value = va_arg(args, unsigned int);
            unsigned base = conversion == 'o' ? 8 : (conversion == 'u' ? 10 : 16);
            emit_unsigned(out, value, base, conversion == 'X', width, precision,
                          left, zero, alternate, 0, conversion == 'p');
        } else if (conversion == 'n') {
            int *count = va_arg(args, int *);
            if (count != NULL) *count = (int)out->length;
        } else {
            return -1;
        }
    }
    if (out->capacity != 0) {
        size_t terminator = out->length < out->capacity ? out->length : out->capacity - 1;
        out->buffer[terminator] = '\0';
    }
    return (int)out->length;
}

int vsnprintf(char *buffer, size_t size, const char *format, va_list args)
{
    output_t out = {buffer, size, 0};
    va_list copy;
    va_copy(copy, args);
    int result = format_to(&out, format, copy);
    va_end(copy);
    return result;
}
int snprintf(char *buffer, size_t size, const char *format, ...)
{
    va_list args; va_start(args, format);
    int result = vsnprintf(buffer, size, format, args);
    va_end(args); return result;
}
int vsprintf(char *buffer, const char *format, va_list args) { return vsnprintf(buffer, (size_t)-1, format, args); }
int sprintf(char *buffer, const char *format, ...)
{
    va_list args; va_start(args, format);
    int result = vsprintf(buffer, format, args);
    va_end(args); return result;
}
int vfprintf(FILE *stream, const char *format, va_list args)
{
    char buffer[1024];
    int length = vsnprintf(buffer, sizeof(buffer), format, args);
    if (length < 0) return -1;
    if ((size_t)length >= sizeof(buffer)) length = sizeof(buffer) - 1;
    return fwrite(buffer, 1, (size_t)length, stream) == (size_t)length ? length : -1;
}
int fprintf(FILE *stream, const char *format, ...)
{
    va_list args; va_start(args, format);
    int result = vfprintf(stream, format, args);
    va_end(args); return result;
}
int printf(const char *format, ...)
{
    va_list args; va_start(args, format);
    int result = vfprintf(stdout, format, args);
    va_end(args); return result;
}
