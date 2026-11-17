/*
 * Project FreeBASIC Coreutils format qualification
 * File coreutils-numfmt-formats.c
 * Purpose Observe both runtime-built numeric output formats.
 * Responsibilities Check native arguments, bounded output and array retirement.
 * Command-line parsing and localized grouping retain their own tests.
 */
#define _GNU_SOURCE 1
#include "fb.h"
#include <limits.h>
#include <locale.h>
#include <math.h>

int c_audit_numfmt(long double, int, char *, size_t, int, int, int, int, long,
    const char *, const char *);
int __wrap_snprintf(char *, size_t, const char *, ...);
int __wrap_sprintf(char *, const char *, ...);
int __real_vsnprintf(char *, size_t, const char *, va_list);
int __real_vsprintf(char *, const char *, va_list);
int __wrap_fb_ArrayRedimEx(FBARRAY_ACCESS *, size_t, int, int, size_t, ...);
int __real_fb_ArrayRedimEx(FBARRAY_ACCESS *, size_t, int, int, size_t, ...);
int __wrap_fb_ArrayErase(FBARRAY_ACCESS *);
int __real_fb_ArrayErase(FBARRAY_ACCESS *);

static unsigned long cases, checks, unscaled_calls, scaled_calls, width_calls, retired;
static FBARRAY_ACCESS *active_array;
static char expected_format[64];
static int expected_precision, expected_scale, expected_width, controlled_result;
static long double expected_number;
static const char *expected_strings[4];
static char *expected_output;
static size_t expected_capacity;

enum { RESULT_NATIVE = INT_MIN };

static void require_at(int condition, int line)
{
    checks++;
    if (!condition) {
        fprintf(stderr, "NUMFMT_FORMAT_FAIL case=%lu check=%lu line=%d scale=%d width=%d format=%s\n",
            cases, checks, line, expected_scale, expected_width, expected_format);
        exit(1);
    }
}

#define require(condition) require_at((condition), __LINE__)

/* Use the exact runtime API, including its initial erase of an empty descriptor.
   Every successful helper call owns one 28-byte, one-dimensional format array. */
int __wrap_fb_ArrayRedimEx(FBARRAY_ACCESS *array, size_t element, int clear,
    int strings, size_t dimensions, ...)
{
    va_list arguments;
    require(active_array == NULL && element == 1 && clear == -1 && strings == 0 && dimensions == 1);
    va_start(arguments, dimensions);
    long long first = va_arg(arguments, long long);
    long long last = va_arg(arguments, long long);
    va_end(arguments);
    require(first == 0 && last == 27 && array->ptr == NULL);
    active_array = array;
    int result = __real_fb_ArrayRedimEx(array, element, clear, strings, dimensions,
        (ssize_t)first, (ssize_t)last);
    require(result == 0 && array->data == array->ptr && array->size == 28 && array->dimensions == 1);
    return result;
}

int __wrap_fb_ArrayErase(FBARRAY_ACCESS *array)
{
    require(array == active_array);
    int allocated = array->ptr != NULL;
    int result = __real_fb_ArrayErase(array);
    require(result == 0 && array->ptr == NULL);
    if (allocated) {
        retired++;
        active_array = NULL;
    }
    return result;
}

int __wrap_sprintf(char *output, const char *format, ...)
{
    va_list arguments, sizing;
    va_start(arguments, format);
    va_copy(sizing, arguments);
    if (active_array) {
        const char *first = active_array->data;
        require((uintptr_t)output >= (uintptr_t)first && (uintptr_t)output - (uintptr_t)first < active_array->size);
        require(strcmp(format, "0%d") == 0 && expected_width > 0);
        va_list checked;
        va_copy(checked, arguments);
        require(va_arg(checked, int) == expected_width);
        va_end(checked);
        int length = __real_vsnprintf(NULL, 0, format, sizing);
        require(length >= 0 && (size_t)length < active_array->size - ((uintptr_t)output - (uintptr_t)first));
        width_calls++;
    }
    int result = __real_vsprintf(output, format, arguments);
    va_end(sizing);
    va_end(arguments);
    return result;
}

int __wrap_snprintf(char *output, size_t capacity, const char *format, ...)
{
    va_list arguments, checked;
    va_start(arguments, format);
    va_copy(checked, arguments);
    if (active_array) {
        require(format == active_array->data && memchr(format, 0, active_array->size) != NULL);
        require(strcmp(format, expected_format) == 0 && output == expected_output && capacity == expected_capacity);
        require(va_arg(checked, int) == expected_precision);
        long double number = va_arg(checked, long double);
        require(number == expected_number && signbit(number) == signbit(expected_number));
        int strings = expected_scale == 0 ? 1 : 4;
        for (int index = 0; index < strings; index++) {
            /* Character-pointer and void-pointer variadic views preserve the
               third string's signed-byte pointer representation as well. */
            const char *text = va_arg(checked, const void *);
            require(text != NULL && strcmp(text, expected_strings[index]) == 0);
        }
        if (expected_scale == 0) unscaled_calls++; else scaled_calls++;
    }
    int result = active_array && controlled_result != RESULT_NATIVE ? controlled_result
        : __real_vsnprintf(output, capacity, format, arguments);
    va_end(checked);
    va_end(arguments);
    return result;
}

static long double rounded_value(long double number, int rounding)
{
    long double result;
    switch (rounding) {
    case 0: result = ceill(number); break;
    case 1: result = floorl(number); break;
    case 2: result = number < 0 ? floorl(number) : ceill(number); break;
    case 3: result = truncl(number); break;
    case 4: result = roundl(number); break;
    default: abort();
    }
    /* The production helper transports its rounded integer through C long. */
    return result == 0 ? 0 : result;
}

static void prepare_reference(long double number, int decimals, int scale, int grouping,
    int rounding, int width, long precision, const char *suffix, const char *separator)
{
    static const char *const powers[] = {"", "K", "M", "G", "T", "P", "E", "Z", "Y", "R", "Q"};
    char digits[32];
    expected_scale = scale;
    expected_width = width;
    strcpy(expected_format, grouping ? "%'" : "%");
    if (width) {
        snprintf(digits, sizeof(digits), "0%d", width);
        strcat(expected_format, digits);
    }
    strcat(expected_format, scale == 0 ? ".*Lf%s" : ".*Lf%s%s%s%s");
    int power = 0;
    if (scale == 0) {
        long double factor = powl(10, decimals);
        expected_number = rounded_value(number * factor, rounding) / factor;
        expected_precision = decimals;
        expected_strings[0] = suffix ? suffix : "";
    } else {
        long double base = scale == 2 ? 1000 : 1024;
        while (fabsl(number) >= base) {
            number /= base;
            power++;
        }
        int digits_after_point = precision < 0 ? (fabsl(number) < 10)
            : (precision < power * 3 ? (int)precision : power * 3);
        long double factor = powl(10, digits_after_point);
        number = rounded_value(number * factor, rounding) / factor;
        if (fabsl(number) >= base) {
            number /= base;
            power++;
        }
        require(power <= 10);
        expected_number = number;
        expected_precision = precision < 0 ? (number != 0 && fabsl(number) < 10 && power > 0) : (int)precision;
        expected_strings[0] = power > 0 && separator ? separator : "";
        expected_strings[1] = scale == 2 && power == 1 ? "k" : powers[power];
        expected_strings[2] = scale == 4 && power > 0 ? "i" : "";
        expected_strings[3] = suffix ? suffix : "";
    }
}

static void numeric_case(long double number, int decimals, int scale, int grouping,
    int rounding, int width, long precision, const char *suffix, const char *separator, int result_control)
{
    unsigned char memory[640];
    char reference[512];
    controlled_result = result_control;
    prepare_reference(number, decimals, scale, grouping, rounding, width, precision, suffix, separator);
    int length = 0;
    if (result_control == RESULT_NATIVE) {
        if (scale == 0)
            length = snprintf(reference, sizeof(reference), expected_format,
                expected_precision, expected_number, expected_strings[0]);
        else
            length = snprintf(reference, sizeof(reference), expected_format, expected_precision,
                expected_number, expected_strings[0], expected_strings[1], expected_strings[2], expected_strings[3]);
        require(length >= 0 && (size_t)length < sizeof(reference));
    }
    const size_t capacities[] = {0, 1, 5, (size_t)length, (size_t)length + 1, sizeof(reference)};
    for (size_t index = 0; index < sizeof(capacities) / sizeof(capacities[0]); index++) {
        cases++;
        memset(memory, 0xa5, sizeof(memory));
        expected_capacity = capacities[index];
        expected_output = expected_capacity ? (char *)memory + 32 : NULL;
        unsigned long before = retired;
        int result = c_audit_numfmt(number, decimals, expected_output, expected_capacity,
            scale, grouping, rounding, width, precision, suffix, separator);
        require(result == (result_control == RESULT_NATIVE ? length : result_control));
        require(active_array == NULL && retired == before + 1);
        if (result_control == RESULT_NATIVE && expected_capacity) {
            size_t copied = (size_t)length < expected_capacity ? (size_t)length : expected_capacity - 1;
            require(memcmp(expected_output, reference, copied) == 0 && expected_output[copied] == 0);
        }
        for (size_t byte = 0; byte < 32; byte++) require(memory[byte] == 0xa5);
        for (size_t byte = 32 + expected_capacity; byte < sizeof(memory); byte++) require(memory[byte] == 0xa5);
    }
}

int main(void)
{
    static const long double numbers[] = {-1000000, -1024, -1023.5L, -1000, -999.95L,
        -99.95L, -9.95L, -1.25L, -.1L, -0.0L, 0, .1L, 1.25L, 9.95L, 99.95L,
        999.95L, 1000, 1023.5L, 1024, 1000000};
    static const int scales[] = {0, 2, 3, 4}, decimal_places[] = {0, 1, 2, 6};
    static const long precisions[] = {-1, 0, 1, 3, 6};
    static const int widths[] = {0, 1, 6, 32, 96};
    static const char *const suffixes[] = {NULL, "", "B", "bytes%"};
    static const char *const separators_text[] = {NULL, "", " ", "_"};
    require(setlocale(LC_ALL, "C") != NULL);
    unsigned long combination = 0;
    for (size_t value = 0; value < sizeof(numbers) / sizeof(numbers[0]); value++)
        for (size_t scale = 0; scale < sizeof(scales) / sizeof(scales[0]); scale++)
            for (int rounding = 0; rounding < 5; rounding++)
                for (size_t decimals = 0; decimals < sizeof(decimal_places) / sizeof(decimal_places[0]); decimals++)
                    for (size_t precision = 0; precision < sizeof(precisions) / sizeof(precisions[0]); precision++) {
                        unsigned long choice = combination++;
                        numeric_case(numbers[value], decimal_places[decimals], scales[scale], (int)(choice % 2), rounding,
                            widths[choice % 5], precisions[precision], suffixes[(choice / 2) % 4],
                            separators_text[(choice / 8) % 4], RESULT_NATIVE);
                    }
    for (size_t scale = 1; scale < sizeof(scales) / sizeof(scales[0]); scale++) {
        long double base = scales[scale] == 2 ? 1000 : 1024;
        long double magnitude = 1;
        for (int power = 0; power <= 10; power++) {
            for (int rounding = 0; rounding < 5; rounding++)
                for (int sign = -1; sign <= 1; sign += 2)
                    numeric_case((long double)sign * magnitude * 1.25L, 2, scales[scale], 1, rounding, 32,
                        -1, "B", " ", RESULT_NATIVE);
            magnitude *= base;
        }
    }
    for (size_t scale = 0; scale < sizeof(scales) / sizeof(scales[0]); scale++) {
        numeric_case(1.25L, 2, scales[scale], 1, 4, 0, -1, "B", " ", -1);
        /* Check the complete maximum-width format and its arguments without
           asking libc to generate more than two billion padding characters. */
        numeric_case(1.25L, 2, scales[scale], 1, 4, INT_MAX, -1, "B", " ", 777);
    }
    require(unscaled_calls > 0 && scaled_calls > 0 && width_calls > 0 && retired == cases);
    printf("NUMFMT_FORMAT_PASS cases=%lu checks=%lu unscaled=%lu scaled=%lu widths=%lu retired=%lu\n",
        cases, checks, unscaled_calls, scaled_calls, width_calls, retired);
    return 0;
}

/* end of coreutils-numfmt-formats.c */
