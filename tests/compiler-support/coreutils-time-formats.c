/*
 * Project: FreeBASIC Coreutils format qualification
 * File: coreutils-time-formats.c
 * Purpose: Observe the production time-format helpers through native libc.
 * Responsibilities: Check format bytes, capacities, timestamps and callback order.
 * Timezone callback policy belongs to the separate gnulib timezone provider.
 */
#define _GNU_SOURCE 1
#include <inttypes.h>
#include <limits.h>
#include <locale.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

size_t __real_strftime(char *, size_t, const char *, const struct tm *);
size_t __wrap_strftime(char *, size_t, const char *, const struct tm *);

static unsigned long cases, checks;
static int events[4], event_count, strftime_calls;
static const char *expected_format;
static size_t expected_capacity;

/* Event 1 enters the provider, 2 formats time, and 3 restores timezone state
   or converts the integer timestamp on the fallback path. */

static void require(int condition)
{
    checks++;
    if (!condition) {
        fprintf(stderr, "TIME_FORMAT_FAIL case=%lu check=%lu format=%s\n",
            cases, checks, expected_format ? expected_format : "unset");
        exit(1);
    }
}

static void event(int value)
{
    require(event_count < 4);
    events[event_count++] = value;
}

size_t __wrap_strftime(char *output, size_t capacity, const char *format, const struct tm *value)
{
    event(2);
    strftime_calls++;
    require(strcmp(format, expected_format) == 0);
    require(capacity == expected_capacity);
    return __real_strftime(output, capacity, format, value);
}

#ifdef TIME_UNDERLYING
unsigned long c_audit_time_call(void *, char *, unsigned long, signed char, signed char, const void *);
void *__wrap_set_tz(void *);
signed char __wrap_revert_tz(void *);

/* Distinct live objects let the observer check the returned timezone token.
   The production helper treats each token as an opaque pointer. */
static unsigned char requested_zone, previous_zone;
static int timezone_mode, set_calls, revert_calls;
enum {
    ZONE_NONE, ZONE_SENTINEL, ZONE_SWITCH, ZONE_SET_FAILURE,
    ZONE_REVERT_FAILURE, ZONE_SENTINEL_REVERT_FAILURE, ZONE_MODE_COUNT
};

void *__wrap_set_tz(void *zone)
{
    event(1);
    set_calls++;
    require(zone == &requested_zone);
    return timezone_mode == ZONE_SET_FAILURE ? NULL : &previous_zone;
}

signed char __wrap_revert_tz(void *zone)
{
    event(3);
    revert_calls++;
    require(zone == (timezone_mode == ZONE_SENTINEL || timezone_mode == ZONE_SENTINEL_REVERT_FAILURE
        ? (void *)(uintptr_t)1 : (void *)&previous_zone));
    return (signed char)(timezone_mode != ZONE_REVERT_FAILURE && timezone_mode != ZONE_SENTINEL_REVERT_FAILURE);
}

static void underlying_case(const struct tm *value, signed char modifier, signed char conversion)
{
    char format[5] = {' ', '%', 0, 0, 0};
    size_t end = 2;
    if (modifier)
        format[end++] = (char)modifier;
    format[end++] = (char)conversion;
    format[end] = '\0';
    for (size_t capacity = 0; capacity <= 65; capacity++) {
        for (int mode = ZONE_NONE; mode < ZONE_MODE_COUNT; mode++) {
            /* Sixteen guard bytes precede and follow the largest output span. */
            unsigned char observed[97], reference[97];
            size_t expected;
            unsigned long actual;
            void *zone = &requested_zone;
            if (mode == ZONE_NONE)
                zone = NULL;
            else if (mode == ZONE_SENTINEL || mode == ZONE_SENTINEL_REVERT_FAILURE)
                zone = (void *)(uintptr_t)1;
            cases++;
            memset(observed, 0x5a, sizeof(observed));
            memset(reference, 0x5a, sizeof(reference));
            expected = __real_strftime((char *)reference + 16, capacity, format, value);
            if (mode == ZONE_SET_FAILURE)
                memset(reference, 0x5a, sizeof(reference));
            if (mode == ZONE_SET_FAILURE || mode == ZONE_REVERT_FAILURE || mode == ZONE_SENTINEL_REVERT_FAILURE)
                expected = 0;
            timezone_mode = mode;
            event_count = strftime_calls = set_calls = revert_calls = 0;
            expected_format = format;
            expected_capacity = capacity;
            actual = c_audit_time_call(zone, (char *)observed + 16, (unsigned long)capacity,
                modifier, conversion, value);
            require(actual == expected);
            require(memcmp(observed, reference, sizeof(observed)) == 0);
            require(set_calls == (mode >= ZONE_SWITCH && mode <= ZONE_REVERT_FAILURE));
            require(strftime_calls == (mode != ZONE_SET_FAILURE));
            require(revert_calls == (mode != ZONE_NONE && mode != ZONE_SET_FAILURE));
            if (mode == ZONE_SET_FAILURE) {
                require(event_count == 1 && events[0] == 1);
            } else if (mode == ZONE_NONE) {
                require(event_count == 1 && events[0] == 2);
            } else if (mode == ZONE_SENTINEL || mode == ZONE_SENTINEL_REVERT_FAILURE) {
                require(event_count == 2 && events[0] == 2 && events[1] == 3);
            } else {
                require(event_count == 3 && events[0] == 1 && events[1] == 2 && events[2] == 3);
            }
        }
    }
}

static void run_cases(void)
{
    static const int64_t samples[] = {-2208988800LL, -1, 0, 951782400,
        1615687199, 1615705200, 2147483647, 253402300799LL};
    const char *ordinary = "aAbBcCdDeFgGhHIjmMpPrRStTuUVwWxXyYzZ%";
    const char *era = "cCxXyY";
    const char *alternate = "deHImMSuUVwWy";
    for (size_t sample = 0; sample < sizeof(samples) / sizeof(samples[0]); sample++) {
        time_t seconds = (time_t)samples[sample];
        struct tm value;
        require(localtime_r(&seconds, &value) != NULL);
        for (size_t index = 0; ordinary[index]; index++)
            underlying_case(&value, 0, (signed char)ordinary[index]);
        for (size_t index = 0; era[index]; index++)
            underlying_case(&value, 'E', (signed char)era[index]);
        for (size_t index = 0; alternate[index]; index++)
            underlying_case(&value, 'O', (signed char)alternate[index]);
    }
}
#else
const char *c_audit_time_string(long, const char *);
struct tm *__real_localtime(const time_t *);
struct tm *__wrap_localtime(const time_t *);
char *__real_imaxtostr(intmax_t, char *);
char *__wrap_imaxtostr(intmax_t, char *);

static int force_localtime_failure, localtime_calls, integer_calls;
static time_t expected_seconds;
static char *integer_result;

struct tm *__wrap_localtime(const time_t *value)
{
    event(1);
    localtime_calls++;
    require(*value == expected_seconds);
    return force_localtime_failure ? NULL : __real_localtime(value);
}

char *__wrap_imaxtostr(intmax_t value, char *buffer)
{
    event(3);
    integer_calls++;
    require(value == (intmax_t)expected_seconds);
    integer_result = __real_imaxtostr(value, buffer);
    return integer_result;
}

static void run_cases(void)
{
    static const int64_t samples[] = {INT64_MIN, -67768040609740800LL,
        -2208988800LL, -2147483648LL, -1, 0, 1, 86400, 951782400,
        1582934400, 1615687199, 1615705200, 1636264799, 1636264800,
        2147483647, 2147483648LL, 4102444800LL, 253402300799LL,
        67768036191676799LL, INT64_MAX};
    static const char *formats[] = {"%Y-%m-%d %H:%M", "%b %e %H:%M"};
    for (size_t sample = 0; sample < sizeof(samples) / sizeof(samples[0]); sample++) {
        for (size_t format = 0; format < sizeof(formats) / sizeof(formats[0]); format++) {
            for (int forced = 0; forced < 2; forced++) {
                char reference[33];
                struct tm value;
                const char *actual;
                int converted;
                cases++;
                expected_seconds = (time_t)samples[sample];
                expected_format = formats[format];
                expected_capacity = 33;
                force_localtime_failure = forced;
                converted = !forced && localtime_r(&expected_seconds, &value) != NULL;
                if (converted) {
                    size_t length = __real_strftime(reference, sizeof(reference), expected_format, &value);
                    require(length > 0 && length < sizeof(reference));
                } else {
                    int length = snprintf(reference, sizeof(reference), "%jd", (intmax_t)expected_seconds);
                    require(length > 0 && (size_t)length < sizeof(reference));
                }
                event_count = strftime_calls = localtime_calls = integer_calls = 0;
                integer_result = NULL;
                actual = c_audit_time_string((long)expected_seconds, expected_format);
                require(actual != NULL && strnlen(actual, 33) < 33);
                require(strcmp(actual, reference) == 0);
                require(localtime_calls == 1);
                require(strftime_calls == converted);
                require(integer_calls == !converted);
                require(event_count == 2 && events[0] == 1 && events[1] == (converted ? 2 : 3));
                if (!converted)
                    require(actual == integer_result);
            }
        }
    }
}
#endif

int main(void)
{
    static const char *locales[] = {"C", "C.UTF-8"};
    static const char *zones[] = {"UTC0", "EST5EDT,M3.2.0,M11.1.0"};
    unsigned available = 0;
    _Static_assert(sizeof(time_t) == sizeof(long) && sizeof(long) == 8,
        "The emitted production profile uses Linux LP64 time values");
    for (size_t locale = 0; locale < sizeof(locales) / sizeof(locales[0]); locale++) {
        if (!setlocale(LC_ALL, locales[locale]))
            continue;
        available++;
        for (size_t zone = 0; zone < sizeof(zones) / sizeof(zones[0]); zone++) {
            require(setenv("TZ", zones[zone], 1) == 0);
            tzset();
            run_cases();
        }
    }
    require(available >= 1 && cases > 0);
    printf("TIME_FORMAT_PASS cases=%lu checks=%lu locales=%u\n", cases, checks, available);
    return 0;
}

/* end of coreutils-time-formats.c */
