#ifndef MAIOS_STDIO_H
#define MAIOS_STDIO_H

#include <stddef.h>
#include <stdarg.h>
#include <sys/types.h>

typedef struct maios_FILE FILE;
extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

#define EOF (-1)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#define BUFSIZ 1024

int remove(const char *path);
int rename(const char *old_path, const char *new_path);
int fflush(FILE *stream);
int fclose(FILE *stream);
void rewind(FILE *stream);
FILE *fopen(const char *path, const char *mode);
FILE *fdopen(int fd, const char *mode);
int fileno(FILE *stream);
int fseek(FILE *stream, long offset, int whence);
long ftell(FILE *stream);
size_t fread(void *ptr, size_t size, size_t count, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t count, FILE *stream);
int fgetc(FILE *stream);
int getc(FILE *stream);
int fputc(int c, FILE *stream);
int ungetc(int c, FILE *stream);
char *fgets(char *buffer, int size, FILE *stream);
int feof(FILE *stream);
int ferror(FILE *stream);
void clearerr(FILE *stream);
int puts(const char *text);
int fputs(const char *text, FILE *stream);
int putchar(int character);
int printf(const char *format, ...);
int sprintf(char *buffer, const char *format, ...);
int fprintf(FILE *stream, const char *format, ...);
int vfprintf(FILE *stream, const char *format, va_list args);
int vsprintf(char *buffer, const char *format, va_list args);
int snprintf(char *buffer, size_t size, const char *format, ...);
int vsnprintf(char *buffer, size_t size, const char *format, va_list args);
int setvbuf(FILE *stream, char *buffer, int mode, size_t size);
void setbuf(FILE *stream, char *buffer);

#define _IONBF 2
#define _IOLBF 1
#define _IOFBF 0

#endif
