/*
    Project FreeBASIC Coreutils callback qualification
    File coreutils-readonly-slots.c
    Compare actual BASIC callbacks with native string and hash results.
    Check borrowed slot identity and use the comparator through native qsort.
    Environment installation and login-file reading have separate coverage.
*/
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int fb_test_slots_compare(const void *, const void *);
signed char fb_test_slots_environment(const void *, const void *);
unsigned long fb_test_slots_hash(const void *, unsigned long);
signed char memeq(const void *, const void *, unsigned long);

/* One serial process owns these buffers. The callbacks borrow their addresses. */
static unsigned long cases, checks, comparisons, hashes;

static void check(int condition)
{
    checks++;
    assert(condition);
}

signed char memeq(const void *a, const void *b, unsigned long length)
{
    return (signed char)(memcmp(a, b, (size_t)length) == 0);
}

static unsigned long reference_hash(const unsigned char *data, size_t length)
{
    unsigned long value = 0;
    size_t index;
    /* The measured port's captured provider uses an LP64 nine-bit rotation. */
    assert(sizeof(unsigned long) * CHAR_BIT == 64);
    for (index = 0; index < length; index++) {
        value = ((value << 9) | (value >> (64 - 9))) + data[index];
    }
    return value;
}

int main(void)
{
    enum { RECORD_COUNT = 128, RECORD_BYTES = 96 };
    char records[RECORD_COUNT][RECORD_BYTES];
    char original[RECORD_COUNT][RECORD_BYTES];
    char *slots[RECORD_COUNT];
    const unsigned long table_sizes[] = { 1, 2, 3, 7, 31, 257, ULONG_MAX };
    unsigned i, j;
    memset(records, 0, sizeof(records));
    for (i = 0; i < RECORD_COUNT; i++) {
        unsigned length = i % 64;
        unsigned column;
        for (column = 0; column < length; column++) {
            unsigned byte = 1 + (i * 19 + column * 13) % 255;
            if (byte == '=') byte++;
            ((unsigned char *)records[i])[column] = (unsigned char)byte;
        }
        records[i][length] = '=';
        records[i][length + 1] = (char)('A' + i % 26);
        slots[i] = records[i];
    }
    memcpy(original, records, sizeof(records));
    for (i = 0; i < RECORD_COUNT; i++) {
        const char *equal = strchr(records[i], '=');
        size_t name_length = (size_t)(equal - records[i]);
        unsigned table;
        for (table = 0; table < sizeof(table_sizes) / sizeof(table_sizes[0]); table++) {
            unsigned long actual = fb_test_slots_hash(&slots[i], table_sizes[table]);
            check(actual == reference_hash((const unsigned char *)records[i], name_length) % table_sizes[table]);
            check(slots[i] == records[i]);
            hashes++;
            cases++;
        }
        for (j = 0; j < RECORD_COUNT; j++) {
            int actual = fb_test_slots_compare(&slots[i], &slots[j]);
            int expected = strcmp(records[i], records[j]);
            const char *other_equal = strchr(records[j], '=');
            size_t other_length = (size_t)(other_equal - records[j]);
            int same_name = name_length == other_length && memcmp(records[i], records[j], name_length) == 0;
            check((actual > 0) == (expected > 0) && (actual < 0) == (expected < 0));
            check((fb_test_slots_environment(&slots[i], &slots[j]) != 0) == same_name);
            check(slots[i] == records[i] && slots[j] == records[j]);
            comparisons++;
            cases++;
        }
        /* A changed value keeps the same environment name and pointer slot. */
        {
            char changed[RECORD_BYTES];
            char *changed_slot = changed;
            memcpy(changed, records[i], sizeof(changed));
            changed[name_length + 1] = 'Z';
            check(fb_test_slots_environment(&slots[i], &changed_slot) != 0);
            check(changed_slot == changed && slots[i] == records[i]);
            cases++;
        }
    }
    qsort(slots, RECORD_COUNT, sizeof(slots[0]), fb_test_slots_compare);
    for (i = 1; i < RECORD_COUNT; i++) check(strcmp(slots[i - 1], slots[i]) <= 0);
    check(memcmp(records, original, sizeof(records)) == 0);
    cases++;
    printf("COREUTILS_READONLY_SLOTS_PASS cases=%lu checks=%lu comparisons=%lu hashes=%lu\n",
           cases, checks, comparisons, hashes);
    return 0;
}
/* end of coreutils-readonly-slots.c */
