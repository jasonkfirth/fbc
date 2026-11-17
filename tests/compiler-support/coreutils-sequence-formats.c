/*
 * Project FreeBASIC Coreutils format qualification
 * File coreutils-sequence-formats.c
 * Purpose Check the configured sequence format and its actual output helpers.
 * Responsibilities Observe parser layouts, native arguments, output and errors.
 * Argument processing and the integer-only fast path retain separate tests.
 */
#define _GNU_SOURCE 1
#include <errno.h>
#include <float.h>
#include <inttypes.h>
#include <locale.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *c_audit_sequence_format(const char *, size_t *, size_t *);
void c_audit_sequence_print(const char *, size_t, size_t, long double,
    long double, long double, const char *, int);
int __wrap_printf(const char *, ...);
int __wrap_asprintf(char **, const char *, ...);
int __wrap_fputs_unlocked(const char *, FILE *);
char *__wrap_setlocale(int, const char *);
void __wrap_free(void *);
void __real_free(void *);
char *__real_setlocale(int, const char *);
int __real_vasprintf(char **, const char *, va_list);
void *xmalloc(size_t);
_Noreturn void xalloc_die(void);
const char *quote(const char *);
char *dcgettext(const char *, const char *, int);
_Noreturn void error(int, int, const char *, ...);
int fpurge(FILE *);

static unsigned long cases, checks, formatted, rounded, failures;
static unsigned long parse_calls, separators, locale_calls;
static jmp_buf error_return;
static int expected_failure, injected_failure, locale_phase;
static const char *expected_source, *expected_format, *expected_separator;
static long double values[32];
static size_t value_count, seen_values, actual_length;
static char actual[32768], reference[32768];
static void *allocations[8];
static char translation[256];

enum { FAIL_NONE, FAIL_PARSE, FAIL_PRINT, FAIL_SEPARATOR, FAIL_ASPRINTF, FAIL_ALLOCATE };
/* Fixed-point output for LDBL_MAX needs thousands of digits on x87 targets.
   Reserve additional room for precision, prefixes, signs and punctuation. */
enum { SEQUENCE_LINE_CAPACITY = LDBL_MAX_10_EXP + 1024 };

_Static_assert(sizeof(long) == 8 && sizeof(long double) == 16,
    "Linux LP64 native long-double profile required");

static void require_at(int condition, int line)
{
    checks++;
    if (!condition) {
        fprintf(stderr, "SEQUENCE_FORMAT_FAIL case=%lu check=%lu line=%d failure=%d seen=%zu\n",
            cases, checks, line, expected_failure, seen_values);
        exit(1);
    }
}

#define require(condition) require_at((condition), __LINE__)

/* Only the parser result and the two rounding probes allocate in this path.
   Keep each address until its actual production free or terminal error. */
static void remember(void *address)
{
    require(address != NULL);
    for (size_t index = 0; index < sizeof(allocations) / sizeof(allocations[0]); index++) {
        if (allocations[index] == NULL) {
            allocations[index] = address;
            return;
        }
    }
    require(0);
}

void __wrap_free(void *address)
{
    if (address == NULL)
        return;
    for (size_t index = 0; index < sizeof(allocations) / sizeof(allocations[0]); index++) {
        if (allocations[index] == address) {
            allocations[index] = NULL;
            __real_free(address);
            return;
        }
    }
    require(0);
}

void *xmalloc(size_t bytes)
{
    require(bytes > 0 && bytes < 512);
    if (injected_failure == FAIL_ALLOCATE)
        xalloc_die();
    void *address = malloc(bytes);
    remember(address);
    return address;
}

_Noreturn void xalloc_die(void)
{
    require(expected_failure == FAIL_ASPRINTF || expected_failure == FAIL_ALLOCATE);
    failures++;
    longjmp(error_return, 1);
}

const char *quote(const char *text)
{
    require(text == expected_source && expected_failure == FAIL_PARSE);
    return text;
}

char *dcgettext(const char *domain, const char *text, int category)
{
    require(domain == NULL && category == 5 && strlen(text) < sizeof(translation));
    strcpy(translation, text);
    return translation;
}

_Noreturn void error(int status, int number, const char *format, ...)
{
    require(status == 1 && format != NULL && expected_failure != FAIL_NONE);
    require(number == (expected_failure == FAIL_PARSE ? 0 : EIO));
    failures++;
    longjmp(error_return, 1);
}

/* The real write-error helper purges stdout before terminating. The observer
   stores output in its own array, so there is no native stream data to purge. */
int fpurge(FILE *stream)
{
    require(stream == stdout && (expected_failure == FAIL_PRINT || expected_failure == FAIL_SEPARATOR));
    return 0;
}

static void append(char *output, size_t *length, const char *text)
{
    size_t bytes = strlen(text);
    require(bytes < sizeof(actual) - *length);
    memcpy(output + *length, text, bytes + 1);
    *length += bytes;
}

int __wrap_printf(const char *format, ...)
{
    va_list arguments;
    char line[SEQUENCE_LINE_CAPACITY];
    va_start(arguments, format);
    long double value = va_arg(arguments, long double);
    va_end(arguments);
    require(format == expected_format && seen_values < value_count);
    require(value == values[seen_values] && signbit(value) == signbit(values[seen_values]));
    seen_values++;
    formatted++;
    if (injected_failure == FAIL_PRINT) {
        errno = EIO;
        return -1;
    }
    int length = snprintf(line, sizeof(line), format, value);
    require(length >= 0 && (size_t)length < sizeof(line));
    append(actual, &actual_length, line);
    return length;
}

int __wrap_asprintf(char **output, const char *format, ...)
{
    va_list arguments;
    require(format == expected_format && seen_values > 0 && seen_values <= value_count);
    rounded++;
    if (injected_failure == FAIL_ASPRINTF)
        return -1;
    va_start(arguments, format);
    int length = __real_vasprintf(output, format, arguments);
    va_end(arguments);
    require(length >= 0 && length < SEQUENCE_LINE_CAPACITY);
    remember(*output);
    return length;
}

int __wrap_fputs_unlocked(const char *text, FILE *stream)
{
    require(stream == stdout && (text == expected_separator || strcmp(text, "\n") == 0));
    separators++;
    if (injected_failure == FAIL_SEPARATOR) {
        errno = EIO;
        return EOF;
    }
    append(actual, &actual_length, text);
    return 1;
}

char *__wrap_setlocale(int category, const char *text)
{
    require(category == LC_NUMERIC);
    require(strcmp(text, locale_phase ? "" : "C") == 0);
    locale_phase = !locale_phase;
    locale_calls++;
    char *result = __real_setlocale(category, text);
    require(result != NULL);
    return result;
}

/* Decode literal percent pairs independently of the production layout walk. */
static size_t decoded_length(const char *first, const char *last)
{
    size_t length = 0;
    while (first < last) {
        if (*first++ == '%') {
            require(first < last && *first == '%');
            first++;
        }
        length++;
    }
    return length;
}

static void reference_sequence(const char *format, size_t prefix, size_t suffix,
    long double first, long double step, long double last, const char *separator)
{
    char line[SEQUENCE_LINE_CAPACITY], previous[SEQUENCE_LINE_CAPACITY];
    size_t length = 0;
    value_count = 0;
    reference[0] = 0;
    long double next = first;
    while (step < 0 ? next >= last : next <= last) {
        require(value_count < sizeof(values) / sizeof(values[0]) - 1);
        if (value_count)
            append(reference, &length, separator);
        values[value_count++] = next;
        int bytes = snprintf(previous, sizeof(previous), format, next);
        require(bytes >= 0 && (size_t)bytes < sizeof(previous));
        append(reference, &length, previous);
        next = first + (long double)value_count * step;
    }
    if (value_count) {
        int bytes = snprintf(line, sizeof(line), format, next);
        require(bytes >= 0 && (size_t)bytes < sizeof(line) && (size_t)bytes >= prefix + suffix);
        line[(size_t)bytes - suffix] = 0;
        previous[strlen(previous) - suffix] = 0;
        char *end;
        long double parsed = strtold(line + prefix, &end);
        if (end != line + prefix && *end == 0 && parsed == last && strcmp(line, previous) != 0) {
            values[value_count++] = next;
            append(reference, &length, separator);
            bytes = snprintf(line, sizeof(line), format, next);
            require(bytes >= 0 && (size_t)bytes < sizeof(line));
            append(reference, &length, line);
        }
        append(reference, &length, "\n");
    }
}

static void sequence_case(const char *format, size_t prefix, size_t suffix,
    long double first, long double step, long double last, const char *separator, int locale)
{
    cases++;
    expected_failure = FAIL_NONE;
    injected_failure = FAIL_NONE;
    expected_format = format;
    expected_separator = separator;
    reference_sequence(format, prefix, suffix, first, step, last, separator);
    actual[0] = 0;
    actual_length = seen_values = 0;
    locale_phase = 0;
    c_audit_sequence_print(format, prefix, suffix, first, step, last, separator, locale);
    require(seen_values == value_count && strcmp(actual, reference) == 0 && locale_phase == 0);
    /* The caller still owns its normalized format until the whole matrix for
       that format has finished. All rounding-probe allocations have retired. */
    for (size_t index = 1; index < sizeof(allocations) / sizeof(allocations[0]); index++)
        require(allocations[index] == NULL);
}

static void invalid_case(const char *source)
{
    size_t prefix, suffix;
    cases++;
    expected_source = source;
    expected_failure = FAIL_PARSE;
    injected_failure = FAIL_NONE;
    if (setjmp(error_return) == 0) {
        c_audit_sequence_format(source, &prefix, &suffix);
        require(0);
    }
    for (size_t index = 0; index < sizeof(allocations) / sizeof(allocations[0]); index++)
        require(allocations[index] == NULL);
}

static void failure_case(int fault)
{
    size_t prefix, suffix;
    const char *format = c_audit_sequence_format("%Lg", &prefix, &suffix);
    cases++;
    expected_failure = injected_failure = fault;
    expected_format = format;
    expected_separator = "|";
    reference_sequence(format, prefix, suffix, 0, 1, 2, expected_separator);
    actual_length = seen_values = 0;
    actual[0] = 0;
    if (setjmp(error_return) == 0) {
        c_audit_sequence_print(format, prefix, suffix, 0, 1, 2, expected_separator, 0);
        require(0);
    }
    __wrap_free((void *)format);
    expected_failure = injected_failure = FAIL_NONE;
}

int main(void)
{
    static const char *const flags[] = {"", "-", "+", "#", "0", " ", "'"};
    static const char *const widths[] = {"", "1", "12", "48"};
    static const char *const precisions[] = {"", ".0", ".3", ".12"};
    static const char conversions[] = "efgaEFGA";
    static const char *const prefixes[] = {"", "pre%%:"};
    static const char *const separators_text[] = {"\n", " | "};
    const long double tiny = nextafterl(0, 1);
    const long double intervals[][3] = {{0, 1, 3}, {3, -1, 0}, {0, .25L, 1},
        {1, -.25L, 0}, {-3, .5L, 0}, {0, 1, -1}, {0, -1, 1}, {-0.0L, 1, 0},
        {LDBL_MAX, LDBL_MAX, LDBL_MAX}, {0, tiny, 3 * tiny},
        {0, 1.25L, 2}, {2, -1.25L, 0}};
    char source[128], normalized[128];
    require(__real_setlocale(LC_ALL, "C") != NULL);
    for (size_t flag = 0; flag < sizeof(flags) / sizeof(flags[0]); flag++)
        for (size_t width = 0; width < sizeof(widths) / sizeof(widths[0]); width++)
            for (size_t precision = 0; precision < sizeof(precisions) / sizeof(precisions[0]); precision++)
                for (size_t conversion = 0; conversion < sizeof(conversions) - 1; conversion++)
                    for (size_t prefix_kind = 0; prefix_kind < sizeof(prefixes) / sizeof(prefixes[0]); prefix_kind++)
                        for (int modifier = 0; modifier < 2; modifier++) {
                            snprintf(source, sizeof(source), "%s%%%s%s%s%s%c:end%%%%", prefixes[prefix_kind],
                                flags[flag], widths[width], precisions[precision], modifier ? "L" : "", conversions[conversion]);
                            snprintf(normalized, sizeof(normalized), "%s%%%s%s%sL%c:end%%%%", prefixes[prefix_kind],
                                flags[flag], widths[width], precisions[precision], conversions[conversion]);
                            size_t prefix, suffix;
                            cases++;
                            expected_source = source;
                            expected_failure = injected_failure = FAIL_NONE;
                            const char *format = c_audit_sequence_format(source, &prefix, &suffix);
                            parse_calls++;
                            require(strcmp(format, normalized) == 0);
                            require(prefix == decoded_length(prefixes[prefix_kind], prefixes[prefix_kind] + strlen(prefixes[prefix_kind])));
                            require(suffix == strlen(":end%"));
                            for (size_t interval = 0; interval < sizeof(intervals) / sizeof(intervals[0]); interval++)
                                for (int locale = 0; locale < 2; locale++)
                                    sequence_case(format, prefix, suffix, intervals[interval][0], intervals[interval][1],
                                        intervals[interval][2], separators_text[locale], locale);
                            __wrap_free((void *)format);
                        }
    static const char *const invalid[] = {"", "literal", "%%", "%", "%L", "%d", "%s", "%n", "%p", "%*f",
        "%.*f", "%lf", "%llf", "%qf", "%g%g", "%g%", "%\200", "%2$g"};
    for (size_t index = 0; index < sizeof(invalid) / sizeof(invalid[0]); index++)
        invalid_case(invalid[index]);
    for (int fault = FAIL_PRINT; fault <= FAIL_ASPRINTF; fault++)
        failure_case(fault);
    cases++;
    expected_failure = injected_failure = FAIL_ALLOCATE;
    if (setjmp(error_return) == 0) {
        size_t prefix, suffix;
        c_audit_sequence_format("%g", &prefix, &suffix);
        require(0);
    }
    for (size_t index = 0; index < sizeof(allocations) / sizeof(allocations[0]); index++)
        require(allocations[index] == NULL);
    char report[256];
    snprintf(report, sizeof(report), "SEQUENCE_FORMAT_PASS cases=%lu checks=%lu formats=%lu rounded=%lu errors=%lu parsers=%lu separators=%lu locales=%lu\n",
        cases, checks, formatted, rounded, failures, parse_calls, separators, locale_calls);
    fputs(report, stdout);
    return 0;
}

/* end of coreutils-sequence-formats.c */
