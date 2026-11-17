/*
    Project: FreeBASIC Coreutils formatter regression
    File: coreutils-vasnprintf.c
    Purpose: Compare the translated formatter and argument providers with libc.
    Responsibilities: Exercise typed dispatch, output buffers and allocation cleanup.
    Production formatting, parsing and va_list extraction belong to the linked modules.
*/
#include <errno.h>
#include <float.h>
#include <inttypes.h>
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <wchar.h>

_Static_assert(sizeof(void *) == 8 && sizeof(long) == 8,
    "The production module packet uses the GNU/Linux LP64 ABI");

char *vasnprintf(char *, size_t *, const char *, va_list);
typedef void (*error_handler)(void);
error_handler fb_ErrorThrowEx(int, int, const char *, void *, void *);
void *__real_malloc(size_t);
void *__real_realloc(void *, size_t);
void __real_free(void *);
void *__wrap_malloc(size_t);
void *__wrap_realloc(void *, size_t);
void __wrap_free(void *);

/* The observer runs serially. Hooks track only calls inside the port, with
   a bounded table that exceeds the live allocations of these small formats. */
static struct {
    void *pointers[128];
    unsigned attempts;
    unsigned fail_at;
    unsigned refusals;
    int active;
    int bad_operation;
} allocations;
static unsigned buffer_mode;
static unsigned requested_failure;
static unsigned long cases;
static unsigned long checks;
static unsigned last_attempts;
static unsigned recoveries;
static unsigned fatal_allocations;

static int pointer_slot(const void *pointer)
{
    for (size_t i = 0; i < sizeof(allocations.pointers) / sizeof(allocations.pointers[0]); i++)
        if (allocations.pointers[i] == pointer)
            return (int)i;
    return -1;
}

static void remember(void *pointer)
{
    if (pointer != NULL) {
        int slot = pointer_slot(NULL);
        if (slot < 0)
            abort();
        allocations.pointers[(unsigned)slot] = pointer;
    }
}

static int refuse_allocation(void)
{
    allocations.attempts++;
    if (allocations.fail_at != 0 && allocations.attempts == allocations.fail_at) {
        allocations.refusals++;
        errno = ENOMEM;
        return 1;
    }
    return 0;
}

void *__wrap_malloc(size_t size)
{
    if (allocations.active && refuse_allocation())
        return NULL;
    void *result = __real_malloc(size);
    if (allocations.active)
        remember(result);
    return result;
}

void *__wrap_realloc(void *pointer, size_t size)
{
    int slot = -1;
    if (allocations.active) {
        if (pointer != NULL) {
            slot = pointer_slot(pointer);
            if (slot < 0) {
                allocations.bad_operation = 1;
                return NULL;
            }
        }
        if (refuse_allocation())
            return NULL;
    }
    void *result = __real_realloc(pointer, size);
    if (allocations.active && (result != NULL || size == 0)) {
        if (slot >= 0)
            allocations.pointers[(unsigned)slot] = result;
        else
            remember(result);
    }
    return result;
}

void __wrap_free(void *pointer)
{
    if (allocations.active && pointer != NULL) {
        int slot = pointer_slot(pointer);
        if (slot < 0) {
            allocations.bad_operation = 1;
            return;
        }
        allocations.pointers[(unsigned)slot] = NULL;
    }
    __real_free(pointer);
}

error_handler fb_ErrorThrowEx(int error, int line, const char *file, void *resume, void *next)
{
    (void)resume;
    (void)next;
    /* C2FB implements automatic storage with guarded heap allocations. Its
       allocation failure terminates through ERROR 7. Each injected failure
       runs in a child so this process-exit contract remains observable. */
    if (error == 7 && requested_failure != 0 && allocations.refusals == 1)
        _Exit(77);
    fprintf(stderr, "UNEXPECTED_FB_ERROR %d %d %s\n", error, line, file);
    abort();
}

static void check(int condition, const char *format)
{
    checks++;
    if (!condition) {
        fprintf(stderr, "VASNPRINTF_FAIL case=%lu check=%lu mode=%u fail=%u format=%s\n",
            cases, checks, buffer_mode, requested_failure, format);
        exit(1);
    }
}

static void run_format(const char *format, const char *reference, ...)
{
    /* A separate red zone surrounds the maximum caller-owned buffer. Bytes
       beyond its supplied capacity also remain checked after each call. */
    struct {
        unsigned char before[16];
        char data[512];
        unsigned char after[16];
    } supplied;
    static const size_t capacities[] = {0, 0, 1, 16, 512};
    char expected[4096];
    size_t capacity = capacities[buffer_mode];
    size_t length = capacity;
    char *buffer = buffer_mode == 0 ? NULL : supplied.data;
    va_list port_arguments, reference_arguments;
    va_start(port_arguments, reference);
    va_copy(reference_arguments, port_arguments);
    int expected_length = vsnprintf(expected, sizeof(expected), reference, reference_arguments);
    va_end(reference_arguments);
    check(expected_length >= 0 && (size_t)expected_length < sizeof(expected), format);
    memset(&supplied, 0xa5, sizeof(supplied));
    memset(&allocations, 0, sizeof(allocations));
    allocations.fail_at = requested_failure;
    allocations.active = 1;
    errno = 0;
    char *actual = vasnprintf(buffer, &length, format, port_arguments);
    int result_errno = errno;
    va_end(port_arguments);
    cases++;
    last_attempts = allocations.attempts;
    check(!allocations.bad_operation, format);
    if (requested_failure != 0)
        check(allocations.refusals == 1, format);
    if (actual == NULL) {
        check(requested_failure != 0 && result_errno == ENOMEM, format);
    } else {
        if (requested_failure != 0)
            recoveries++;
        check(actual != NULL && length == (size_t)expected_length, format);
        check(memcmp(actual, expected, length + 1) == 0, format);
        check((actual == buffer) == (buffer != NULL && (size_t)expected_length < capacity), format);
    }
    if (actual != NULL && actual != buffer)
        __wrap_free(actual);
    for (size_t i = 0; i < sizeof(allocations.pointers) / sizeof(allocations.pointers[0]); i++)
        check(allocations.pointers[i] == NULL, format);
    allocations.active = 0;
    for (size_t i = 0; i < sizeof(supplied.before); i++) {
        check(supplied.before[i] == 0xa5, format);
        check(supplied.after[i] == 0xa5, format);
    }
    for (size_t i = capacity; i < sizeof(supplied.data); i++)
        check((unsigned char)supplied.data[i] == 0xa5, format);
}

static intmax_t signed_value(unsigned seed, intmax_t minimum, intmax_t maximum)
{
    static const intmax_t middle[] = {0, -1, 0, 1};
    if (seed == 0)
        return minimum;
    if (seed == 4)
        return maximum;
    return middle[seed];
}

static uintmax_t unsigned_value(unsigned seed, uintmax_t maximum)
{
    switch (seed) {
    case 0: return 0;
    case 1: return 1;
    case 2: return maximum;
    case 3: return (maximum >> 1) + 1;
    default: return maximum >> 1;
    }
}

static void value_case(unsigned type, unsigned seed, unsigned stars, int width, int precision)
{
    static const char *const directives[] = {
        "hhd", "hhu", "hd", "hu", "d", "u", "ld", "lu", "lld", "llu",
        "w8d", "w8u", "w16d", "w16u", "w32d", "w32u", "w64d", "w64u",
        "wf8d", "wf8u", "wf16d", "wf16u", "wf32d", "wf32u", "wf64d", "wf64u",
        "g", "Lg", "c", "lc", "s", "ls", "p"
    };
    static const char *const references[] = {
        "hhd", "hhu", "hd", "hu", "d", "u", "ld", "lu", "lld", "llu",
        "hhd", "hhu", "hd", "hu", "d", "u", "ld", "lu",
        "hhd", "hhu", "ld", "lu", "ld", "lu", "ld", "lu",
        "g", "Lg", "c", "lc", "s", "ls", "p"
    };
    static const char *const modifiers[] = {"", "*", ".*", "*.*"};
    static const double doubles[] = {0.0, -0.0, 1.25, -1.5, DBL_MIN, DBL_MAX, INFINITY, NAN};
    static const long double extended[] = {0.0L, -0.0L, 1.25L, -1.5L, LDBL_MIN, LDBL_MAX, INFINITY, NAN};
    static const int characters[] = {0, 'A', 0x7f, 0xff, ' '};
    static const wint_t wide_characters[] = {0, L'A', 0xe9, 0x3a9, 0x1f600};
    static const char *const strings[] = {"", "abc", "two words", "literal %d", "caf\xc3\xa9"};
    static const wchar_t *const wide_strings[] = {L"", L"abc", L"two words", L"caf\u00e9", L"\U0001f600"};
    static int pointer_value;
    char format[48], reference[48];
    snprintf(format, sizeof(format), "[%%%s%s]", modifiers[stars], directives[type - 1]);
    snprintf(reference, sizeof(reference), "[%%%s%s]", modifiers[stars], references[type - 1]);
#define VALUE(value) do { \
    if (stars == 0) run_format(format, reference, (value)); \
    else if (stars == 1) run_format(format, reference, width, (value)); \
    else if (stars == 2) run_format(format, reference, precision, (value)); \
    else run_format(format, reference, width, precision, (value)); \
} while (0)
    switch (type) {
    case 1: VALUE((signed char)signed_value(seed, SCHAR_MIN, SCHAR_MAX)); break;
    case 2: VALUE((unsigned char)unsigned_value(seed, UCHAR_MAX)); break;
    case 3: VALUE((short)signed_value(seed, SHRT_MIN, SHRT_MAX)); break;
    case 4: VALUE((unsigned short)unsigned_value(seed, USHRT_MAX)); break;
    case 5: VALUE((int)signed_value(seed, INT_MIN, INT_MAX)); break;
    case 6: VALUE((unsigned int)unsigned_value(seed, UINT_MAX)); break;
    case 7: VALUE((long)signed_value(seed, LONG_MIN, LONG_MAX)); break;
    case 8: VALUE((unsigned long)unsigned_value(seed, ULONG_MAX)); break;
    case 9: VALUE((long long)signed_value(seed, LLONG_MIN, LLONG_MAX)); break;
    case 10: VALUE((unsigned long long)unsigned_value(seed, ULLONG_MAX)); break;
    case 11: VALUE((int8_t)signed_value(seed, INT8_MIN, INT8_MAX)); break;
    case 12: VALUE((uint8_t)unsigned_value(seed, UINT8_MAX)); break;
    case 13: VALUE((int16_t)signed_value(seed, INT16_MIN, INT16_MAX)); break;
    case 14: VALUE((uint16_t)unsigned_value(seed, UINT16_MAX)); break;
    case 15: VALUE((int32_t)signed_value(seed, INT32_MIN, INT32_MAX)); break;
    case 16: VALUE((uint32_t)unsigned_value(seed, UINT32_MAX)); break;
    case 17: VALUE((int64_t)signed_value(seed, INT64_MIN, INT64_MAX)); break;
    case 18: VALUE((uint64_t)unsigned_value(seed, UINT64_MAX)); break;
    case 19: VALUE((int_fast8_t)signed_value(seed, INT_FAST8_MIN, INT_FAST8_MAX)); break;
    case 20: VALUE((uint_fast8_t)unsigned_value(seed, UINT_FAST8_MAX)); break;
    case 21: VALUE((int_fast16_t)signed_value(seed, INT_FAST16_MIN, INT_FAST16_MAX)); break;
    case 22: VALUE((uint_fast16_t)unsigned_value(seed, UINT_FAST16_MAX)); break;
    case 23: VALUE((int_fast32_t)signed_value(seed, INT_FAST32_MIN, INT_FAST32_MAX)); break;
    case 24: VALUE((uint_fast32_t)unsigned_value(seed, UINT_FAST32_MAX)); break;
    case 25: VALUE((int_fast64_t)signed_value(seed, INT_FAST64_MIN, INT_FAST64_MAX)); break;
    case 26: VALUE((uint_fast64_t)unsigned_value(seed, UINT_FAST64_MAX)); break;
    case 27: VALUE(doubles[seed]); break;
    case 28: VALUE(extended[seed]); break;
    case 29: VALUE(characters[seed]); break;
    case 30: VALUE(wide_characters[seed]); break;
    case 31: VALUE(strings[seed]); break;
    case 32: VALUE(wide_strings[seed]); break;
    case 33: VALUE(seed == 0 ? NULL : (void *)&pointer_value); break;
    default: abort();
    }
#undef VALUE
}

int main(void)
{
    static const int widths[] = {-17, -1, 0, 1, 7, 31};
    static const int precisions[] = {-1, 0, 1, 5, 17, 3};
    check(setlocale(LC_ALL, "C.UTF-8") != NULL, "locale");
    for (unsigned type = 1; type <= 33; type++)
        for (unsigned stars = 0; stars < 4; stars++)
            for (unsigned option = 0; option < (stars == 0 ? 1u : 6u); option++)
                for (unsigned seed = 0; seed < (type == 27 || type == 28 ? 8u : 5u); seed++)
                    for (buffer_mode = 0; buffer_mode < 5; buffer_mode++)
                        value_case(type, seed, stars, widths[option], precisions[option]);
    buffer_mode = 0;
    run_format("%d/%s/%lld/%u/%ld/%c/%g/%s/%d", "%d/%s/%lld/%u/%ld/%c/%g/%s/%d",
        -7, "first", 19LL, 23u, -31L, 'Q', 1.5, "last", 41);
    unsigned attempts = last_attempts;
    for (requested_failure = 1; requested_failure <= attempts; requested_failure++) {
        pid_t child = fork();
        check(child >= 0, "allocation child");
        if (child == 0) {
            run_format("%d/%s/%lld/%u/%ld/%c/%g/%s/%d", "%d/%s/%lld/%u/%ld/%c/%g/%s/%d",
                -7, "first", 19LL, 23u, -31L, 'Q', 1.5, "last", 41);
            _Exit(recoveries != 0 ? 78 : 0);
        }
        int status;
        check(waitpid(child, &status, 0) == child && WIFEXITED(status), "allocation child status");
        int result = WEXITSTATUS(status);
        check(result == 0 || result == 77 || result == 78, "allocation failure disposition");
        fatal_allocations += result == 77;
        recoveries += result == 78;
        cases++;
    }
    printf("VASNPRINTF_MATRIX_PASS cases=%lu checks=%lu allocation_failures=%u recoveries=%u fatal_allocations=%u\n", cases, checks, attempts, recoveries, fatal_allocations);
    return 0;
}

/* end of coreutils-vasnprintf.c */
