/*
 * Project: FreeBASIC Coreutils format qualification
 * File: coreutils-idle-formats.c
 * Purpose: Check the production pinky and who idle-time formatting ranges.
 * Responsibilities: Observe native arguments, clock caching and output bytes.
 * Application entry points and terminal discovery retain their original owners.
 */
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *c_audit_idle_call(long, long);
long __wrap_rpl_time(long *);
char *__wrap_dcgettext(const char *, const char *, int);
int __wrap_sprintf(char *, const char *, ...);
int __real_sprintf(char *, const char *, ...) __attribute__((format(printf, 2, 3)));

_Static_assert(sizeof(long) == sizeof(int64_t), "Linux LP64 timestamp required");
_Static_assert(sizeof(int) == 4, "Native C integer width required");

static long clock_value;
static unsigned long cases, checks, clock_calls, format_calls, translation_calls;
static unsigned long maximum_formatted_length;
static int expected_hours, expected_minutes, expect_days;
static intmax_t expected_days;
static char old_translation[] = " old ";

static void require(int condition)
{
    checks++;
    if (!condition) {
        fprintf(stderr, "IDLE_FORMAT_FAIL case=%lu check=%lu\n", cases, checks);
        exit(1);
    }
}

/* Both helpers cache their first clock reading. Their original sentinel values
   deliberately cause another reading when the supplied clock equals a sentinel. */
long __wrap_rpl_time(long *destination)
{
    require(destination != NULL);
    clock_calls++;
    *destination = clock_value;
    return clock_value;
}

char *__wrap_dcgettext(const char *domain, const char *message, int category)
{
    require(domain == NULL);
    require(category == 5);
    require(strcmp(message, " old ") == 0);
    translation_calls++;
    return old_translation;
}

/* Read each argument using the declared C variadic ABI before invoking libc.
   The bounded argument checks run before sprintf writes the production buffer.
   The 6-byte who buffer holds HH:MM and its terminator. Pinky owns 22 bytes. */
int __wrap_sprintf(char *output, const char *format, ...)
{
    va_list arguments;
    int length;
    require(output != NULL);
    format_calls++;
    va_start(arguments, format);
    if (expect_days) {
        intmax_t days = va_arg(arguments, intmax_t);
        require(strcmp(format, "%jdd") == 0);
        require(days == expected_days && days > 0);
        length = __real_sprintf(output, "%jdd", days);
        require(length > 0 && length < 22);
    } else {
        int hours = va_arg(arguments, int);
        int minutes = va_arg(arguments, int);
        require(strcmp(format, "%02d:%02d") == 0);
        require(hours == expected_hours && hours >= 0 && hours < 24);
        require(minutes == expected_minutes && minutes >= 0 && minutes < 60);
        length = __real_sprintf(output, "%02d:%02d", hours, minutes);
        require(length == 5);
    }
    va_end(arguments);
    if (length > 0 && (unsigned long)length > maximum_formatted_length)
        maximum_formatted_length = (unsigned long)length;
    return length;
}

/* The emitted pinky profile uses -fwrapv for signed BASIC subtraction.
   Copy unsigned subtraction bits into a signed object for an independent model. */
static long wrapped_difference(long first, long second)
{
    uint64_t bits = (uint64_t)first - (uint64_t)second;
    long result;
    memcpy(&result, &bits, sizeof(result));
    return result;
}

static long access_for_delta(long delta)
{
    return wrapped_difference(clock_value, delta);
}

static void idle_case(long access, long boot)
{
    char expected[22];
    const char *actual;
    unsigned long previous_formats = format_calls;
    unsigned long previous_translations = translation_calls;
    int formatted = 0, translated = 0;
    expect_days = 0;
    cases++;
#ifdef IDLE_PINKY
    long delta = wrapped_difference(clock_value, access);
    if (delta < 60) {
        memcpy(expected, "     ", 6);
    } else if (delta < 86400) {
        expected_hours = (int)(delta / 3600);
        expected_minutes = (int)((delta % 3600) / 60);
        require(snprintf(expected, sizeof(expected), "%02d:%02d",
            expected_hours, expected_minutes) == 5);
        formatted = 1;
    } else {
        expect_days = 1;
        expected_days = delta / 86400;
        int length = snprintf(expected, sizeof(expected), "%jdd", expected_days);
        require(length > 0 && (size_t)length < sizeof(expected));
        formatted = 1;
    }
#else
    uint64_t delta = (uint64_t)clock_value - (uint64_t)access;
    if (boot < access && access <= clock_value && delta < 86400) {
        if (delta < 60) {
            memcpy(expected, "  .  ", 6);
        } else {
            expected_hours = (int)(delta / 3600);
            expected_minutes = (int)((delta % 3600) / 60);
            require(snprintf(expected, sizeof(expected), "%02d:%02d",
                expected_hours, expected_minutes) == 5);
            formatted = 1;
        }
    } else {
        memcpy(expected, old_translation, sizeof(old_translation));
        translated = 1;
    }
#endif
    actual = c_audit_idle_call(access, boot);
    require(actual != NULL && strcmp(actual, expected) == 0);
    require(format_calls - previous_formats == (unsigned long)formatted);
    require(translation_calls - previous_translations == (unsigned long)translated);
    if (translated)
        require(actual == old_translation);
#ifdef IDLE_PINKY
    require(clock_calls == (clock_value == 0 ? cases : 1));
#else
    require(clock_calls == (clock_value == LONG_MIN ? cases : 1));
#endif
}

static void access_cases(long access)
{
    idle_case(access, LONG_MIN);
    idle_case(access, access);
    idle_case(access, LONG_MAX);
    if (access != LONG_MIN)
        idle_case(access, access - 1);
}

int main(int argc, char **argv)
{
    static const long clocks[] = {0, 1, -1, LONG_MIN, LONG_MIN + 1, LONG_MAX,
        1000000000000000000L};
    static const long boundaries[] = {LONG_MIN, LONG_MIN + 1, -86401, -86400,
        -60, -1, 86402, INT_MAX, (long)INT_MAX + 1, LONG_MAX - 1, LONG_MAX};
    require(argc == 2);
    require(strlen(argv[1]) == 1 && argv[1][0] >= '0' && argv[1][0] <= '6');
    clock_value = clocks[(unsigned int)(argv[1][0] - '0')];
    for (long delta = 0; delta <= 86401; delta++)
        access_cases(access_for_delta(delta));
    for (size_t index = 0; index < sizeof(boundaries) / sizeof(boundaries[0]); index++) {
        access_cases(boundaries[index]);
        access_cases(access_for_delta(boundaries[index]));
    }
    printf("IDLE_FORMAT_PASS cases=%lu checks=%lu max_length=%lu\n",
        cases, checks, maximum_formatted_length);
    return 0;
}

/* end of coreutils-idle-formats.c */
