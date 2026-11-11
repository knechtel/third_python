#ifndef MAIOS_ASSERT_H
#define MAIOS_ASSERT_H

void __maios_assert_fail(const char *expression, const char *file,
                         unsigned int line, const char *function);

#ifdef NDEBUG
#define assert(expression) ((void)0)
#else
#define assert(expression) \
    ((expression) ? (void)0 : __maios_assert_fail(#expression, __FILE__, \
                                                   __LINE__, __func__))
#endif

#endif
