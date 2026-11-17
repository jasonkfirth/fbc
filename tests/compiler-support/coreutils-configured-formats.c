/*
 * Project: FreeBASIC Coreutils format qualification
 * File: coreutils-configured-formats.c
 * Purpose: Observe configured line-number and split-file format contracts.
 * Responsibilities: Check parser decisions, argument types, bounds and output.
 * Complete application startup and filesystem operations retain their owners.
 */
#define _GNU_SOURCE 1
#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned long cases, checks, format_calls, error_calls;
static jmp_buf error_return;
static int expect_error;
static char translated[256];

char *__wrap_dcgettext(const char *, const char *, int);
_Noreturn void __wrap_error(int, int, const char *, ...);
int __real_vsnprintf(char *, size_t, const char *, va_list);
int __real_vsprintf(char *, const char *, va_list);
int __real_snprintf(char *, size_t, const char *, ...);

_Static_assert(sizeof(long) == 8 && sizeof(int) == 4, "Linux LP64 profile required");

static void require(int condition)
{
    checks++;
    if (!condition) {
        fprintf(stderr, "CONFIGURED_FORMAT_FAIL case=%lu check=%lu\n", cases, checks);
        exit(1);
    }
}

/* These are synchronous provider controls. Error status one terminates the
   production operation, represented here by returning to its test checkpoint. */
char *__wrap_dcgettext(const char *domain, const char *message, int category)
{
    size_t length = strlen(message);
    require(domain == NULL && category == 5 && length < sizeof(translated));
    memcpy(translated, message, length + 1);
    return translated;
}

_Noreturn void __wrap_error(int status, int number, const char *format, ...)
{
    require(expect_error && status == 1 && number == 0 && format != NULL);
    error_calls++;
    longjmp(error_return, 1);
}

#ifdef CONFIGURED_NL
void c_audit_numbered_line(int, int, long, long, const char *, int, long *, int *);
int __wrap_printf(const char *, ...);

static const char *const line_formats[] = {"%-*jd%s", "%*jd%s", "%0*jd%s"};
static int expected_mode, expected_width;
static intmax_t expected_line;
static const char *expected_separator;
static char actual_output[512];

int __wrap_printf(const char *format, ...)
{
    va_list arguments, output_arguments;
    int width, length;
    intmax_t value;
    const char *separator;
    require(!expect_error && strcmp(format, line_formats[expected_mode]) == 0);
    va_start(arguments, format);
    va_copy(output_arguments, arguments);
    width = va_arg(arguments, int);
    value = va_arg(arguments, intmax_t);
    separator = va_arg(arguments, const char *);
    require(width == expected_width && value == expected_line && separator == expected_separator);
    length = __real_vsnprintf(actual_output, sizeof(actual_output), format, output_arguments);
    require(length >= 0 && (size_t)length < sizeof(actual_output));
    va_end(output_arguments);
    va_end(arguments);
    format_calls++;
    return length;
}

static void numbered_case(int mode, int width, long first, long increment, const char *separator)
{
    char reference[512];
    long next, expected_next;
    int overflow;
    uint64_t bits = (uint64_t)first + (uint64_t)increment;
    int expected_overflow = (increment > 0 && first > LONG_MAX - increment)
        || (increment < 0 && first < LONG_MIN - increment);
    unsigned long before = format_calls;
    memcpy(&expected_next, &bits, sizeof(expected_next));
    expected_mode = mode;
    expected_width = width;
    expected_line = first;
    expected_separator = separator;
    expect_error = 0;
    cases++;
    c_audit_numbered_line(mode, width, first, increment, separator, 0, &next, &overflow);
    switch (mode) {
    case 0: snprintf(reference, sizeof(reference), "%-*jd%s", width, (intmax_t)first, separator); break;
    case 1: snprintf(reference, sizeof(reference), "%*jd%s", width, (intmax_t)first, separator); break;
    case 2: snprintf(reference, sizeof(reference), "%0*jd%s", width, (intmax_t)first, separator); break;
    default: abort();
    }
    require(format_calls == before + 1 && strcmp(actual_output, reference) == 0);
    require(next == expected_next);
    require((overflow != 0) == expected_overflow);
    expect_error = 1;
    if (setjmp(error_return) == 0) {
        c_audit_numbered_line(mode, width, next, increment, separator, 1, &next, &overflow);
        require(0);
    }
    require(format_calls == before + 1);
}

int main(void)
{
    static const long numbers[] = {LONG_MIN, LONG_MIN + 1, -1000000, -1, 0, 1,
        9, 10, 999999, LONG_MAX - 1, LONG_MAX};
    static const int widths[] = {1, 2, 6, 31, 128};
    static const char *const separators[] = {"", "\t", " | ", "percent%data", "\xc3\xa9"};
    for (int mode = 0; mode < 3; mode++)
        for (size_t width = 0; width < sizeof(widths) / sizeof(widths[0]); width++)
            for (size_t first = 0; first < sizeof(numbers) / sizeof(numbers[0]); first++)
                for (size_t step = 0; step < sizeof(numbers) / sizeof(numbers[0]); step++)
                    for (size_t separator = 0; separator < sizeof(separators) / sizeof(separators[0]); separator++)
                        numbered_case(mode, widths[width], numbers[first], numbers[step], separators[separator]);
    require(error_calls == cases);
    char report[256];
    snprintf(report, sizeof(report), "CONFIGURED_FORMAT_PASS cases=%lu checks=%lu formats=%lu errors=%lu\n",
        cases, checks, format_calls, error_calls);
    fputs(report, stdout);
    return 0;
}
#else
long c_audit_split_validate(char *);
char *c_audit_split_filename(char *, const char *, char *, int, int);
int __wrap_sprintf(char *, const char *, ...);
int __wrap_snprintf(char *, size_t, const char *, ...);
int8_t c_isdigit(int);
char *quotearg_style(int, const char *);
char *quotearg_n_style_colon(int, int, const char *);

static const char *expected_format;
static int expected_index, expected_digits, custom_suffix;
static char *expected_destination;
static size_t destination_capacity;

int __wrap_snprintf(char *output, size_t capacity, const char *format, ...)
{
    va_list arguments, checked;
    int result;
    require(!expect_error && output == NULL && capacity == 0);
    require(strcmp(format, expected_format) == 0);
    va_start(arguments, format);
    va_copy(checked, arguments);
    require(va_arg(checked, int) == INT_MAX);
    result = __real_vsnprintf(output, capacity, format, arguments);
    va_end(checked);
    va_end(arguments);
    return result;
}

/* The gnulib classifier has an ASCII contract independent of the locale.
   The format parser itself is the unchanged production helper. */
int8_t c_isdigit(int value)
{
    return value >= '0' && value <= '9';
}

/* MAX_OUT retains the allocation-failure cleanup graph in its object file.
   These providers belong to filesystem error reporting. Reaching either one
   invalidates this bounded formatter test before any file operation occurs. */
char *quotearg_style(int style, const char *name)
{
    (void)style;
    (void)name;
    require(0);
    abort();
}

char *quotearg_n_style_colon(int slot, int style, const char *name)
{
    (void)slot;
    (void)style;
    (void)name;
    require(0);
    abort();
}

int __wrap_sprintf(char *output, const char *format, ...)
{
    va_list arguments, sizing_arguments, output_arguments;
    int number, length, result;
    require(output == expected_destination && strcmp(format, expected_format) == 0);
    va_start(arguments, format);
    va_copy(sizing_arguments, arguments);
    va_copy(output_arguments, arguments);
    if (!custom_suffix)
        require(va_arg(arguments, int) == expected_digits);
    number = va_arg(arguments, int);
    require(number == expected_index && number >= 0);
    length = __real_vsnprintf(NULL, 0, format, sizing_arguments);
    require(length >= 0 && (size_t)length < destination_capacity);
    result = __real_vsprintf(output, format, output_arguments);
    require(result == length);
    va_end(output_arguments);
    va_end(sizing_arguments);
    va_end(arguments);
    format_calls++;
    return result;
}

/* Parse independently, then normalize the one allowed unsigned decimal form.
   The source program numbers its output files using nonnegative C integers. */
static int reference_format(char *format)
{
    size_t conversions = 0;
    for (size_t index = 0; format[index]; index++) {
        unsigned int flags = 0;
        char conversion;
        if (format[index] != '%')
            continue;
        index++;
        if (format[index] == '%')
            continue;
        if (++conversions != 1)
            return 0;
        while (format[index] && strchr("-0'#", format[index])) {
            if (format[index] == '\'') flags |= 1;
            if (format[index] == '#') flags |= 2;
            index++;
        }
        while (format[index] >= '0' && format[index] <= '9') index++;
        if (format[index] == '.') {
            index++;
            while (format[index] >= '0' && format[index] <= '9') index++;
        }
        conversion = format[index];
        if (!conversion || !strchr("diuoxX", conversion))
            return 0;
        if (flags & ~(strchr("diu", conversion) ? 1u : 2u))
            return 0;
        if (conversion == 'u') format[index] = 'd';
    }
    return conversions == 1;
}

static void filename_case(const char *format, const char *prefix, int digits, int number)
{
    unsigned char memory[1024];
    char suffix[256], normalized[256], reference[512];
    long maximum;
    size_t capacity, prefix_length = strlen(prefix);
    unsigned long before = format_calls;
    custom_suffix = format != NULL;
    expect_error = 0;
    cases++;
    if (format) {
        require(strlen(format) < sizeof(suffix));
        strcpy(suffix, format);
        strcpy(normalized, format);
        expect_error = !reference_format(normalized);
        expected_format = normalized;
        if (setjmp(error_return) != 0) {
            require(expect_error && format_calls == before);
            return;
        }
        maximum = c_audit_split_validate(suffix);
        require(!expect_error && strcmp(suffix, normalized) == 0);
        require(maximum == __real_snprintf(NULL, 0, normalized, INT_MAX));
        capacity = prefix_length + (size_t)maximum + 1;
        expected_format = suffix;
        __real_snprintf(reference, sizeof(reference), normalized, number);
    } else {
        capacity = prefix_length + (size_t)(digits > 10 ? digits : 10) + 1;
        expected_format = "%0*d";
        __real_snprintf(reference, sizeof(reference), "%0*d", digits, number);
    }
    /* Thirty-two guard bytes precede the exact production output capacity. */
    require(capacity < sizeof(memory) - 64);
    memset(memory, 0xa5, sizeof(memory));
    expected_destination = (char *)memory + 32 + prefix_length;
    destination_capacity = capacity - prefix_length;
    expected_index = number;
    expected_digits = digits;
    char *returned = c_audit_split_filename((char *)memory + 32, prefix,
        format ? suffix : NULL, digits, number);
    require(returned == (char *)memory + 32);
    require(memcmp(returned, prefix, prefix_length) == 0);
    require(strcmp(returned + prefix_length, reference) == 0);
    require(format_calls == before + 1);
    for (size_t index = 0; index < 32; index++) require(memory[index] == 0xa5);
    for (size_t index = 32 + capacity; index < sizeof(memory); index++) require(memory[index] == 0xa5);
}

int main(void)
{
    static const char *const flags[] = {"", "0", "-", "-0", "#", "'", "'#", "0#", "0'", "--", "00"};
    static const char *const widths[] = {"", "1", "2", "8", "31", "96"};
    static const char *const precisions[] = {"", ".0", ".1", ".7", ".12"};
    static const char conversions[] = "diuoxXsfnp";
    static const char *const prefixes[] = {"", "xx", "name%", "\xc3\xa9"};
    static const int numbers[] = {0, 1, 9, 10, 255, 65535, INT_MAX};
    char format[128];
    for (size_t flag = 0; flag < sizeof(flags) / sizeof(flags[0]); flag++)
        for (size_t width = 0; width < sizeof(widths) / sizeof(widths[0]); width++)
            for (size_t precision = 0; precision < sizeof(precisions) / sizeof(precisions[0]); precision++)
                for (size_t conversion = 0; conversion < sizeof(conversions) - 1; conversion++) {
                    __real_snprintf(format, sizeof(format), "part%%%%_%%%s%s%s%c_end%%%%",
                        flags[flag], widths[width], precisions[precision], conversions[conversion]);
                    for (size_t number = 0; number < sizeof(numbers) / sizeof(numbers[0]); number++)
                        for (size_t prefix = 0; prefix < sizeof(prefixes) / sizeof(prefixes[0]); prefix++)
                            filename_case(format, prefixes[prefix], 2, numbers[number]);
                }
    static const char *const invalid[] = {"", "literal", "%%", "%", "%d%d", "%s", "%*d", "%ld", "% d", "%+d", "%\200"};
    for (size_t index = 0; index < sizeof(invalid) / sizeof(invalid[0]); index++)
        filename_case(invalid[index], "prefix", 2, 0);
    for (int digits = 0; digits <= 32; digits++)
        for (size_t number = 0; number < sizeof(numbers) / sizeof(numbers[0]); number++)
            filename_case(NULL, "default", digits, numbers[number]);
    printf("CONFIGURED_FORMAT_PASS cases=%lu checks=%lu formats=%lu errors=%lu\n",
        cases, checks, format_calls, error_calls);
    return 0;
}
#endif

/* end of coreutils-configured-formats.c */
