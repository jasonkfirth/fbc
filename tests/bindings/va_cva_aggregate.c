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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

/* Keep crash progress on the same unbuffered diagnostic path. A native CI
   runner may report only a signal number, without a debugger or core file. */
void cva_test_checkpoint(const char *stage)
{
    fprintf(stderr, "CVA_STAGE=%s\n", stage);
    fflush(stderr);
}

static int64_t decode_native_aggregates(va_list arguments)
{
    /* Mirror the BASIC checksum using integral-valued floating fixtures.
       This distinguishes a broken native cursor from the cross-language path. */
    struct AggregateRR rr = va_arg(arguments, struct AggregateRR);
    struct AggregateSmall small = va_arg(arguments, struct AggregateSmall);
    struct AggregateX x = va_arg(arguments, struct AggregateX);
    struct AggregateRX rx = va_arg(arguments, struct AggregateRX);
    struct AggregateXR xr = va_arg(arguments, struct AggregateXR);
    struct AggregateXX xx = va_arg(arguments, struct AggregateXX);
    struct AggregateMemory memory = va_arg(arguments, struct AggregateMemory);

    return rr.first_value * INT64_C(1000000000000) +
           rr.second_value * INT64_C(100000000000) +
           (int64_t)small.first_value * INT64_C(10000000000) +
           (int64_t)small.second_value * INT64_C(1000000000) +
           (int64_t)x.floating_value * INT64_C(100000000) +
           rx.integer_part * INT64_C(10000000) +
           (int64_t)rx.floating_part * INT64_C(1000000) +
           (int64_t)xr.floating_part * INT64_C(100000) + xr.integer_part * INT64_C(10000) +
           (int64_t)xx.first_value * INT64_C(1000) +
           (int64_t)xx.second_value * INT64_C(100) +
           memory.first_value * INT64_C(10) + memory.second_value + memory.third_value;
}

static void check_native_aggregates(va_list arguments, int64_t expected)
{
    va_list reference;
    int64_t actual;

    /* The control must not consume the cursor subsequently passed to BASIC.
       va_copy also works on targets where va_list is a register-save record. */
    va_copy(reference, arguments);
    actual = decode_native_aggregates(reference);
    va_end(reference);
    if (actual != expected) {
        fprintf(stderr, "CVA_NATIVE_ABI_FAILED expected=%lld actual=%lld\n",
                (long long)expected, (long long)actual);
        fflush(stderr);
        /* BASIC owns assertion statuses 1 through 15 in this fixture. */
        exit(16);
    }
}

static int64_t send_aggregates(int marker, ...)
{
    va_list arguments;
    int64_t result;
    (void)marker;
    va_start(arguments, marker);
    check_native_aggregates(arguments, INT64_C(15533436603905));
    cva_test_checkpoint("C-registers-to-BASIC");
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
    check_native_aggregates(arguments, INT64_C(1235534567911));
    cva_test_checkpoint("C-overflow-to-BASIC");
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

void cva_write_longdouble_precision(void *destination)
{
    const long double value = 1.0000000000000001L;
    memcpy(destination, &value, sizeof(value));
}

int cva_longdouble_has_extra_precision(void)
{
    /* Native long double can have the same precision as double. Keep the
       expected comparison consistent with the literal written above. */
    return 1.0000000000000001L > 1.0L;
}

#if defined(__linux__) && (defined(__x86_64__) || defined(__i386__))
long double cva_longdouble_identity(long double value)
{
    return value;
}

long double cva_longdouble_transform(long double value)
{
    return value * 2.0L + 0.5L;
}

long double cva_longdouble_add(long double lhs, long double rhs)
{
    return lhs + rhs;
}

long double fbc_cva_longdouble_export(long double value);

long double cva_call_fbc_longdouble_export(long double value)
{
    return fbc_cva_longdouble_export(value);
}
#endif

/* end of va_cva_aggregate.c */
