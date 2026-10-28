/*
    Project: FreeBASIC compiler tests
    File: va_cva_aggregate.c
    Purpose: Exercise C varargs and fixed-parameter CLongDouble ABI calls.
    Responsibilities: Check aggregate register/overflow classes and native
    long-double argument and return values across the C boundary.
    This file contains no FreeBASIC backend implementation or ABI emulation.
*/

#include <stdarg.h>
#include <stdint.h>

struct AggregateRR {
    int64_t first_value;
    int64_t second_value;
};

struct AggregateSmall {
    int32_t first_value;
    int32_t second_value;
};

struct AggregateX {
    double floating_value;
};

struct AggregateRX {
    int64_t integer_part;
    double floating_part;
};

struct AggregateXR {
    double floating_part;
    int64_t integer_part;
};

struct AggregateXX {
    double first_value;
    double second_value;
};

struct AggregateMemory {
    int64_t first_value;
    int64_t second_value;
    int64_t third_value;
};

int64_t read_cva_aggregates(va_list arguments);
int read_cva_longdouble(va_list arguments);

static int64_t send_aggregates(int marker, ...)
{
    va_list arguments;
    int64_t result;
    (void)marker;
    va_start(arguments, marker);
    result = read_cva_aggregates(arguments);
    va_end(arguments);
    return result;
}

static int64_t send_aggregates_overflow(int first, int second, int third,
                                        int fourth, int fifth, int marker, ...)
{
    va_list arguments;
    int64_t result;
    (void)first;
    (void)second;
    (void)third;
    (void)fourth;
    (void)fifth;
    (void)marker;
    va_start(arguments, marker);
    result = read_cva_aggregates(arguments);
    va_end(arguments);
    return result;
}

static int send_longdouble(int marker, ...)
{
    va_list arguments;
    int result;
    (void)marker;
    va_start(arguments, marker);
    result = read_cva_longdouble(arguments);
    va_end(arguments);
    return result;
}

int64_t cva_aggregate_registers(void)
{
    struct AggregateRR rr = {12, 34};
    struct AggregateSmall small_value = {11, 22};
    struct AggregateX x_value = {13.0};
    struct AggregateRX rx = {11, 25.0};
    struct AggregateXR xr = {12.0, 34};
    struct AggregateXX xx = {56.0, 78.0};
    struct AggregateMemory memory_value = {9, 8, 7};
    return send_aggregates(0, rr, small_value, x_value, rx, xr, xx, memory_value);
}

int64_t cva_aggregate_overflow(void)
{
    struct AggregateRR rr = {1, 2};
    struct AggregateSmall small_value = {3, 4};
    struct AggregateX x_value = {15.0};
    struct AggregateRX rx = {3, 4.0};
    struct AggregateXR xr = {5.0, 6};
    struct AggregateXX xx = {7.0, 8.0};
    struct AggregateMemory memory_value = {9, 10, 11};
    return send_aggregates_overflow(1, 2, 3, 4, 5, 6, rr, small_value, x_value, rx, xr, xx, memory_value);
}

int cva_longdouble_register(void)
{
    return send_longdouble(0, 1.75L);
}

#if defined(__linux__) && defined(__x86_64__)
long double cva_longdouble_identity(long double value)
{
    return value;
}

long double cva_longdouble_transform(long double value)
{
    return value * 2.0L + 0.5L;
}

long double fbc_cva_longdouble_export(long double value);

long double cva_call_fbc_longdouble_export(long double value)
{
    return fbc_cva_longdouble_export(value);
}
#endif

/* end of va_cva_aggregate.c */
