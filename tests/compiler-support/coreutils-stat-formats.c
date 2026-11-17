/*
 * Project: FreeBASIC C-backend qualification
 * File: coreutils-stat-formats.c
 * Purpose: Qualify the production stat format-builder call contracts.
 * Responsibilities: Compare formatted bytes, buffers, return values and callback
 * ownership using bounded inputs and the original emitted helper bodies.
 * The observer supplies deterministic callbacks and a separate libc reference.
 */
#define _GNU_SOURCE 1
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int c_audit_stat_call(int, char *, unsigned long, long, unsigned long, const char *, int);
int __wrap_printf(const char *, ...) __attribute__((format(printf, 1, 2)));
int rpl_getfilecon(const char *, char **);
int rpl_lgetfilecon(const char *, char **);
void freecon(char *);
char *quotearg_style(int, const char *);
void __wrap_error(int, int, const char *, ...) __attribute__((format(printf, 3, 4)));

static unsigned long checked_values;
static char captured[512], captured_format[256];
static int printf_calls, force_output_failure, context_failure;
static int followed, unfollowed, released, reported_errors;
static char context_value[] = "context-value";
static char quoted_name[128];

static void require(int condition)
{
    if (!condition) {
        fprintf(stderr, "STAT_FORMAT_VALUES_FAIL case=%lu format=%s\n", checked_values, captured_format);
        exit(1);
    }
}

int __wrap_printf(const char *format, ...)
{
    va_list arguments;
    int result;
    printf_calls++;
    require(strlen(format) < sizeof(captured_format));
    strcpy(captured_format, format);
    if (force_output_failure) {
        captured[0] = '\0';
        return -1;
    }
    va_start(arguments, format);
    result = vsnprintf(captured, sizeof(captured), format, arguments);
    va_end(arguments);
    return result;
}

static int context_result(char **value)
{
    if (context_failure) {
        errno = EIO;
        return -1;
    }
    *value = strdup(context_value);
    require(*value != NULL);
    return (int)strlen(*value);
}

int rpl_getfilecon(const char *file, char **value)
{
    require(strcmp(file, "fixture-name") == 0);
    followed++;
    return context_result(value);
}

int rpl_lgetfilecon(const char *file, char **value)
{
    require(strcmp(file, "fixture-name") == 0);
    unfollowed++;
    return context_result(value);
}

void freecon(char *value)
{
    require(value != NULL && strcmp(value, context_value) == 0);
    released++;
    free(value);
}

char *quotearg_style(int style, const char *text)
{
    require(style == 4 && strlen(text) < sizeof(quoted_name));
    strcpy(quoted_name, text);
    return quoted_name;
}

void __wrap_error(int status, int error, const char *format, ...)
{
    va_list arguments;
    char message[256];
    require(status == 0 && error == EIO);
    va_start(arguments, format);
    require(vsnprintf(message, sizeof(message), format, arguments) > 0);
    va_end(arguments);
    require(strstr(message, "fixture-name") != NULL);
    reported_errors++;
}

static void reference_format(char *destination, const char *prefix, int kind)
{
    static const char *const allowed[] = {"-", "'-+ 0", "'-0", "-#0", "-#0", "'-+ 0", "-"};
    static const char *const conversion[] = {"s", "jd", "ju", "jo", "jx", ".0f", "s"};
    size_t source = 1, target = 1;
    destination[0] = '%';
    if (kind == 6) {
        target = strlen(prefix);
        memcpy(destination, prefix, target);
    } else {
        while (prefix[source] != '\0' && strchr("'-+ #0I", prefix[source]) != NULL) {
            if (strchr(allowed[kind], prefix[source]) != NULL)
                destination[target++] = prefix[source];
            source++;
        }
        while (prefix[source] != '\0')
            destination[target++] = prefix[source++];
    }
    strcpy(destination + target, conversion[kind]);
}

static void check_value(int kind, const char *prefix, long signed_value,
                        unsigned long unsigned_value, const char *text, int follow)
{
    unsigned char actual_format[256], expected_format[256];
    char expected[sizeof(captured)];
    char *format = (char *)actual_format + 1;
    char *reference = (char *)expected_format + 1;
    size_t prefix_length = strlen(prefix);
    int expected_count, expected_result, observed;
    require(kind >= 0 && kind <= 6 && prefix_length < 64);
    memset(actual_format, 0xa5, sizeof(actual_format));
    memset(expected_format, 0xa5, sizeof(expected_format));
    memcpy(format, prefix, prefix_length + 1);
    memcpy(reference, prefix, prefix_length + 1);
    reference_format(reference, prefix, kind);
    memset(captured, 0xcc, sizeof(captured));
    memset(expected, 0xcc, sizeof(expected));
    captured_format[0] = '\0';
    printf_calls = followed = unfollowed = released = reported_errors = 0;
    if (force_output_failure) {
        expected[0] = '\0';
        expected_count = -1;
    } else if (kind == 0 || kind == 6) {
        const char *value = kind == 6 ? (context_failure ? "?" : context_value) : text;
        expected_count = snprintf(expected, sizeof(expected), reference, value);
    } else if (kind == 1) {
        expected_count = snprintf(expected, sizeof(expected), reference, (intmax_t)signed_value);
    } else if (kind == 5) {
        expected_count = snprintf(expected, sizeof(expected), reference, -0.25);
    } else {
        expected_count = snprintf(expected, sizeof(expected), reference, (uintmax_t)unsigned_value);
    }
    expected_result = kind == 1 || kind == 2 || kind == 5 ? expected_count : 0;
    if (kind == 6)
        expected_result = context_failure;
    observed = c_audit_stat_call(kind, format, (unsigned long)prefix_length,
        signed_value, unsigned_value, kind == 6 ? "fixture-name" : text, follow);
    require(printf_calls == 1);
    require(observed == expected_result);
    require(memcmp(actual_format, expected_format, sizeof(actual_format)) == 0);
    require(memcmp(captured, expected, sizeof(captured)) == 0);
    require(strcmp(captured_format, reference) == 0);
    if (kind == 6) {
        require(followed == follow && unfollowed == !follow);
        require(released == !context_failure && reported_errors == context_failure);
    } else {
        require(followed == 0 && unfollowed == 0 && released == 0 && reported_errors == 0);
    }
    checked_values++;
}

int main(void)
{
    static const char *const widths[] = {"", "1", "4", "12", "0008"};
    static const char *const precisions[] = {"", ".0", ".1", ".8"};
    static const char *const texts[] = {"", "plain", "a%z", "A\xc3\xa9\xe2\x82\xac"};
    static const long signed_values[] = {LONG_MIN, -1000, -1, 0, 1, 42, LONG_MAX};
    static const unsigned long unsigned_values[] = {0, 1, 7, 8, 15, 16, ULONG_MAX};
    char prefix[128], flags[8];
    require(sizeof(long) == 8 && sizeof(void *) == 8);
    require(setlocale(LC_ALL, "C") != NULL);
    for (int mask = 0; mask < 128; mask++) {
        size_t flag_count = 0;
        for (int bit = 0; bit < 7; bit++) {
            if ((mask & (1 << bit)) != 0)
                flags[flag_count++] = "'-+ #0I"[bit];
        }
        flags[flag_count] = '\0';
        for (size_t width = 0; width < sizeof(widths) / sizeof(widths[0]); width++) {
            for (size_t precision = 0; precision < sizeof(precisions) / sizeof(precisions[0]); precision++) {
                require(snprintf(prefix, sizeof(prefix), "%%%s%s%s", flags, widths[width], precisions[precision]) > 0);
                for (size_t text = 0; text < sizeof(texts) / sizeof(texts[0]); text++)
                    check_value(0, prefix, 0, 0, texts[text], 0);
                for (int kind = 1; kind <= 4; kind++) {
                    for (size_t value = 0; value < sizeof(signed_values) / sizeof(signed_values[0]); value++)
                        check_value(kind, prefix, signed_values[value], unsigned_values[value], "", 0);
                }
                /* The production epoch caller removes its precision before
                   asking this helper to print the negative zero component. */
                if (precision == 0)
                    check_value(5, prefix, 0, 0, "", 0);
            }
        }
    }
    for (int left = 0; left < 2; left++) {
        for (size_t width = 0; width < sizeof(widths) / sizeof(widths[0]); width++) {
            /* This helper directly appends its string conversion. Its valid
               string prefixes use space padding or left alignment. */
            if (widths[width][0] == '0')
                continue;
            for (size_t precision = 0; precision < sizeof(precisions) / sizeof(precisions[0]); precision++) {
                require(snprintf(prefix, sizeof(prefix), "%%%s%s%s", left ? "-" : "", widths[width], precisions[precision]) > 0);
                for (int follow = 0; follow < 2; follow++) {
                    for (int failure = 0; failure < 2; failure++) {
                        context_failure = failure;
                        check_value(6, prefix, 0, 0, "", follow);
                    }
                }
            }
        }
    }
    context_failure = 0;
    force_output_failure = 1;
    for (int kind = 0; kind <= 6; kind++)
        check_value(kind, "%4", 42, 42, "plain", 0);
    fprintf(stderr, "STAT_FORMAT_VALUES_PASS %lu\n", checked_values);
    return 0;
}

/* end of coreutils-stat-formats.c */
