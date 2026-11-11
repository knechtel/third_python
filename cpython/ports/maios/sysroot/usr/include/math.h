#ifndef MAIOS_MATH_H
#define MAIOS_MATH_H

#define HUGE_VAL (__builtin_huge_val())
#define INFINITY (__builtin_inff())
#define NAN (__builtin_nanf(""))
#define FP_NAN 0
#define FP_INFINITE 1
#define FP_ZERO 2
#define FP_SUBNORMAL 3
#define FP_NORMAL 4

typedef float float_t;
typedef double double_t;

double fabs(double x);
double floor(double x);
double ceil(double x);
double trunc(double x);
double round(double x);
double rint(double x);
double nearbyint(double x);
double sqrt(double x);
double fmod(double x, double y);
double modf(double x, double *integer);
double frexp(double x, int *exponent);
double ldexp(double x, int exponent);
double copysign(double x, double y);
double cbrt(double x);
double hypot(double x, double y);
double pow(double x, double y);
double exp(double x);
double exp2(double x);
double log(double x);
double log10(double x);
double log2(double x);
double sin(double x);
double cos(double x);
double tan(double x);
double asin(double x);
double acos(double x);
double atan(double x);
double atan2(double y, double x);
double sinh(double x);
double cosh(double x);
double tanh(double x);
double asinh(double x);
double acosh(double x);
double atanh(double x);
double fma(double x, double y, double z);
double fmin(double x, double y);
double fmax(double x, double y);
double fmod(double x, double y);
double remainder(double x, double y);
double remquo(double x, double y, int *quo);
double scalbn(double x, int n);
double nextafter(double x, double y);
double erf(double x);
double erfc(double x);
double expm1(double x);
double log1p(double x);
int isfinite(double x);
int isinf(double x);
int isnan(double x);
int signbit(double x);

#endif
