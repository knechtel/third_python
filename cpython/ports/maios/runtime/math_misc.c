#include <math.h>
#include <stdint.h>

double fabs(double value)
{
    union { double value; unsigned long long bits; } number = { value };
    number.bits &= 0x7fffffffffffffffULL;
    return number.value;
}

double cbrt(double value)
{
    if (value == 0.0 || !isfinite(value)) return value;
    int negative = value < 0.0;
    int exponent;
    double mantissa = frexp(fabs(value), &exponent);
    int scale = exponent / 3;
    int remainder = exponent - scale * 3;
    double estimate = 1.0;
    mantissa = ldexp(mantissa, remainder);
    for (int i = 0; i < 12; ++i)
        estimate = (2.0 * estimate + mantissa / (estimate * estimate)) / 3.0;
    double result = ldexp(estimate, scale);
    return negative ? -result : result;
}

double hypot(double x, double y)
{
    x = fabs(x);
    y = fabs(y);
    if (isinf(x) || isinf(y)) return HUGE_VAL;
    if (isnan(x) || isnan(y)) return NAN;
    if (x < y) {
        double swap = x;
        x = y;
        y = swap;
    }
    if (x == 0.0) return 0.0;
    double ratio = y / x;
    return x * sqrt(1.0 + ratio * ratio);
}

double exp2(double value)
{
    if (isnan(value)) return value;
    if (value >= 1024.0) return HUGE_VAL;
    if (value < -1075.0) return 0.0;
    if (value == floor(value)) return ldexp(1.0, (int)value);
    return exp(value * 0x1.62e42fefa39efp-1);
}

double nextafter(double from, double toward)
{
    union { double value; uint64_t bits; } current = { from };
    if (isnan(from) || isnan(toward)) return from + toward;
    if (from == toward) return toward;
    if (from == 0.0) {
        current.bits = 1;
        current.bits |= ((union { double value; uint64_t bits; }){ toward }).bits
                        & 0x8000000000000000ULL;
        return current.value;
    }
    if ((from > 0.0) == (toward > from)) ++current.bits;
    else --current.bits;
    return current.value;
}
