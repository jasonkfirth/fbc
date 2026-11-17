/*
    Project: FreeBASIC C backend regression
    File: c-bulk-memory-arguments.c
    Purpose: Compare byte-argument lowering with independent native operations.
    Responsibilities: Check guarded records, returned pointers and evaluations.
    Compiler overrun diagnostics have a separate compile-only control.
*/
#include <stddef.h>
#include <stdint.h>
#include <string.h>

struct memory_argument_record {
    uint32_t prefix;
    uint16_t first;
    uint16_t following;
    uint8_t rest[8];
};

struct memory_argument_guard {
    unsigned char before[16];
    struct memory_argument_record value;
    unsigned char after[16];
};

struct memory_argument_large_guard {
    unsigned char before[16];
    unsigned char value[32];
    unsigned char after[16];
};

int probe_memory_argument_count(void);
int probe_memory_argument_fill(void *destination, size_t count, int value);
int probe_memory_argument_fixed_fill(void *destination, int value);
int probe_memory_argument_extended_fill(void *destination, int value);
int probe_memory_argument_copy(void *destination, const void *source, size_t count);
int probe_memory_argument_move(void *destination, size_t count);
int probe_memory_argument_cases(void);
int probe_memory_argument_observe(const void *value);

static int observe_result;

int probe_memory_argument_observe(const void *value)
{
    const unsigned char *bytes = value;
    observe_result = bytes[0];
    return observe_result;
}

int probe_memory_argument_cases(void)
{
    struct memory_argument_guard actual;
    struct memory_argument_guard expected;
    struct memory_argument_record source;
    struct memory_argument_large_guard large_actual;
    struct memory_argument_large_guard large_expected;
    const int values[] = {0, 1, 127, 128, 255, 256, -1};
    size_t count;
    size_t index;
    size_t value_index;

    if (sizeof(source) != 16 || offsetof(struct memory_argument_record, first) != 4 ||
        offsetof(struct memory_argument_record, following) != 6)
        return 0;
    for (index = 0; index < sizeof(source); index++)
        ((unsigned char *)&source)[index] = (unsigned char)(index * 13 + 7);
    if (probe_memory_argument_count() != 0)
        return 0;

    for (count = 0; count <= 12; count++) {
        for (value_index = 0; value_index < sizeof(values) / sizeof(values[0]); value_index++) {
            memset(&actual, 0xA5, sizeof(actual));
            expected = actual;
            memset((unsigned char *)&expected.value + 4, values[value_index], count);
            if (probe_memory_argument_fill(&actual.value, count, values[value_index]) != -1 ||
                probe_memory_argument_count() != 1 || memcmp(&actual, &expected, sizeof(actual)) != 0)
                return 0;
        }
        memset(&actual, 0xA5, sizeof(actual));
        expected = actual;
        memcpy((unsigned char *)&expected.value + 4, (const unsigned char *)&source + 4, count);
        if (probe_memory_argument_copy(&actual.value, &source, count) != -1 ||
            probe_memory_argument_count() != 2 || memcmp(&actual, &expected, sizeof(actual)) != 0)
            return 0;
    }

    for (value_index = 0; value_index < sizeof(values) / sizeof(values[0]); value_index++) {
        memset(&actual, 0xA5, sizeof(actual));
        expected = actual;
        memset((unsigned char *)&expected.value + 4, values[value_index], 12);
        if (probe_memory_argument_fixed_fill(&actual.value, values[value_index]) != -1 ||
            probe_memory_argument_count() != 1 || memcmp(&actual, &expected, sizeof(actual)) != 0)
            return 0;
        memset(&large_actual, 0xA5, sizeof(large_actual));
        large_expected = large_actual;
        memset(large_expected.value + 4, values[value_index], 24);
        if (probe_memory_argument_extended_fill(large_actual.value, values[value_index]) != -1 ||
            probe_memory_argument_count() != 1 || memcmp(&large_actual, &large_expected, sizeof(large_actual)) != 0)
            return 0;
    }

    for (count = 0; count <= 10; count++) {
        memset(&actual, 0xA5, sizeof(actual));
        actual.value = source;
        expected = actual;
        memmove((unsigned char *)&expected.value + 6, (unsigned char *)&expected.value + 4, count);
        if (probe_memory_argument_move(&actual.value, count) != -1 ||
            probe_memory_argument_count() != 2 || memcmp(&actual, &expected, sizeof(actual)) != 0)
            return 0;
    }
    return -1;
}

#ifdef MEMORY_ARGUMENT_NATIVE_MAIN
int main(void)
{
    return probe_memory_argument_cases() == -1 ? 0 : 1;
}
#endif

/* end of c-bulk-memory-arguments.c */
