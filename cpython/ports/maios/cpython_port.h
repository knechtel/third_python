#ifndef MAIOS_CPYTHON_PORT_H
#define MAIOS_CPYTHON_PORT_H

/* Embedded CPython entry points used by the maiOs kernel. */
int maios_cpython_initialize(void);
int maios_cpython_run(const char *source);
int maios_cpython_run_file(const char *filename);
int maios_cpython_repl(void);
int maios_cpython_finalize(void);

#endif
