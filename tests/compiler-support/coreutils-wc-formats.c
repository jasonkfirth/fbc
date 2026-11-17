/*
 * Project: FreeBASIC C-backend qualification
 * File: coreutils-wc-formats.c
 * Purpose: Check the five production word-count format call contracts.
 * Responsibilities: Observe output, integer conversion buffers and I/O callbacks.
 * The complete utility's file traversal has its own application qualification.
 */
#define _GNU_SOURCE 1
#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void c_audit_wc_call(int, int, unsigned long, unsigned long, unsigned long,
    unsigned long, long, const char *);
int __wrap_printf(const char *, ...) __attribute__((format(printf, 1, 2)));
int __wrap_putchar_unlocked(int);
int __wrap_ferror_unlocked(FILE *);
int __wrap_fflush_unlocked(FILE *);
void __wrap_clearerr_unlocked(FILE *);
void __wrap_error(int, int, const char *, ...) __attribute__((format(printf, 3, 4)));
int fpurge(FILE *);
char *umaxtostr(unsigned long, char *);
char *imaxtostr(long, char *);
char *quotearg_n_style_colon(int, int, const char *);

/* Five fields of at most 32 columns and the short filename fit inside this bound. */
static char captured[512];
static size_t captured_bytes;
static unsigned long checks, cases;
static int output_failure, quoted, reported, flushed, purged, cleared;
static int printed, newlines, unsigned_conversions, signed_conversions;
static const char *expected_name;
static char quoted_name[] = "quoted-name";

static void require(int condition)
{
    checks++;
    if (!condition) {
        fprintf(stderr, "WC_FORMAT_VALUES_FAIL case=%lu check=%lu\n", cases, checks);
        exit(1);
    }
}

int __wrap_printf(const char *format, ...)
{
    va_list arguments;
    int result;
    require(strcmp(format, "%*s") == 0 || strcmp(format, " %*s") == 0 ||
        strcmp(format, " %s") == 0);
    printed++;
    if (output_failure)
        return -1;
    va_start(arguments, format);
    result = vsnprintf(captured + captured_bytes, sizeof(captured) - captured_bytes,
        format, arguments);
    va_end(arguments);
    require(result >= 0 && (size_t)result < sizeof(captured) - captured_bytes);
    captured_bytes += (size_t)result;
    return result;
}

int __wrap_putchar_unlocked(int value)
{
    require(value == '\n');
    newlines++;
    if (output_failure)
        return EOF;
    require(captured_bytes + 1 < sizeof(captured));
    captured[captured_bytes++] = (char)value;
    captured[captured_bytes] = '\0';
    return value;
}

int __wrap_ferror_unlocked(FILE *stream)
{
    require(stream == stdout);
    return output_failure;
}

int __wrap_fflush_unlocked(FILE *stream)
{
    require(stream == stdout);
    flushed++;
    errno = EINTR;
    return 0;
}

int fpurge(FILE *stream)
{
    require(stream == stdout);
    purged++;
    errno = EINVAL;
    return 0;
}

void __wrap_clearerr_unlocked(FILE *stream)
{
    require(stream == stdout);
    cleared++;
    /* Cleanup may change errno. The reported failure belongs to the earlier write. */
    errno = ERANGE;
}

void __wrap_error(int status, int error_number, const char *format, ...)
{
    /* Record the requested exit and return so the remaining failure cases run. */
    require(status == 1 && error_number == EIO && strcmp(format, "write error") == 0);
    reported++;
}

/* GNU inttostr returns an interior pointer in the caller's 21-byte buffer.
 * Generate digits backwards so the production call must use that pointer.
 */
static char *convert_integer(unsigned long value, int negative, char *buffer)
{
    char *first = buffer + 20;
    *first = '\0';
    do {
        require(first > buffer);
        *--first = (char)('0' + value % 10);
        value /= 10;
    } while (value != 0);
    if (negative) {
        require(first > buffer);
        *--first = '-';
    }
    return first;
}

char *umaxtostr(unsigned long value, char *buffer)
{
    unsigned_conversions++;
    return convert_integer(value, 0, buffer);
}

char *imaxtostr(long value, char *buffer)
{
    unsigned long magnitude = (unsigned long)value;
    signed_conversions++;
    if (value < 0)
        magnitude = 0UL - magnitude;
    return convert_integer(magnitude, value < 0, buffer);
}

char *quotearg_n_style_colon(int slot, int style, const char *name)
{
    require(slot == 0 && style == 3 && name == expected_name);
    require(strchr(name, '\n') != NULL);
    quoted++;
    return quoted_name;
}

static void reference(char *text, int mask, int width, const unsigned long *values,
    long signed_value, const char *name)
{
    size_t used = 0;
    int columns = 0;
    for (int field = 0; field < 5; field++) {
        int length;
        if ((mask & (1 << field)) == 0)
            continue;
        if (field == 4)
            length = snprintf(text + used, 512 - used, columns ? " %*ld" : "%*ld",
                width, signed_value);
        else
            length = snprintf(text + used, 512 - used, columns ? " %*lu" : "%*lu",
                width, values[field]);
        require(length >= 0 && (size_t)length < 512 - used);
        used += (size_t)length;
        columns++;
    }
    if (name != NULL) {
        const char *shown = strchr(name, '\n') ? quoted_name : name;
        int length = snprintf(text + used, 512 - used, " %s", shown);
        require(length >= 0 && (size_t)length < 512 - used);
        used += (size_t)length;
    }
    require(used + 1 < 512);
    text[used++] = '\n';
    text[used] = '\0';
}

static void check_case(int mask, int width, unsigned long value, long signed_value,
    const char *name, int fail)
{
    unsigned long values[] = {value, ULONG_MAX - value, value ^ 0x123456789abcdef0UL, value / 2};
    char expected[512] = "";
    int unsigned_fields = 0;
    cases++;
    captured[0] = '\0';
    captured_bytes = 0;
    printed = newlines = unsigned_conversions = signed_conversions = 0;
    quoted = reported = flushed = purged = cleared = 0;
    expected_name = name;
    output_failure = fail;
    errno = EIO;
    c_audit_wc_call(mask, width, values[0], values[1], values[2], values[3], signed_value, name);
    for (int field = 0; field < 4; field++)
        unsigned_fields += (mask >> field) & 1;
    require(unsigned_conversions == unsigned_fields);
    require(signed_conversions == ((mask >> 4) & 1));
    require(printed == unsigned_fields + signed_conversions + (name != NULL));
    require(newlines == 1);
    require(quoted == (name != NULL && strchr(name, '\n') != NULL));
    require(reported == fail && flushed == fail && purged == fail && cleared == fail);
    if (!fail)
        reference(expected, mask, width, values, signed_value, name);
    require(strcmp(captured, expected) == 0);
}

int main(void)
{
    static const unsigned long values[] = {0, 1, 9, 10, ULONG_MAX, 4294967296UL, 1234567890123456789UL};
    static const long signed_values[] = {0, 1, -1, LONG_MIN, LONG_MAX, 4294967296L, -1234567890123456789L};
    static const int widths[] = {-24, 0, 1, 7, 20, 32};
    static const char *const names[] = {NULL, "", "plain", "two words", "%s%u", "utf8-\303\251", "line\nbreak"};
    require(sizeof(long) == 8 && sizeof(void *) == 8);
    require(setlocale(LC_ALL, "C") != NULL);
    for (int mask = 0; mask < 32; mask++) {
        for (size_t width = 0; width < sizeof(widths) / sizeof(widths[0]); width++)
            for (size_t value = 0; value < sizeof(values) / sizeof(values[0]); value++)
                for (size_t name = 0; name < sizeof(names) / sizeof(names[0]); name++)
                    check_case(mask, widths[width], values[value], signed_values[value], names[name], 0);
        check_case(mask, 20, ULONG_MAX, LONG_MIN, NULL, 1);
        check_case(mask, 20, ULONG_MAX, LONG_MIN, "line\nbreak", 1);
    }
    require(cases == 9472);
    fprintf(stdout, "WC_FORMAT_VALUES_PASS cases=%lu checks=%lu\n", cases, checks);
    return 0;
}

/* end of coreutils-wc-formats.c */
