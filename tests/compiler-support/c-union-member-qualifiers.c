/*
    Project: FreeBASIC C backend regression
    File: c-union-member-qualifiers.c
    Purpose: Observe overlapping pointer slots through independent native records.
    Responsibilities: Check addresses, layout, stored words and surrounding bytes.
    The BASIC fixture supplies the compiler-generated member operations.
*/
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

union pointer_choice {
    const int32_t *read_only_long;
    int32_t *write_long;
    const int16_t *read_only_short;
    int16_t *write_short;
    void *any_value;
};
struct choice_record {
    uintptr_t prefix;
    union pointer_choice items[2];
    uintptr_t suffix;
};
struct __attribute__((packed)) packed_choice_record {
    unsigned char prefix;
    union pointer_choice items[2];
    uintptr_t suffix;
};

int32_t **probe_union_mutable_slot(struct choice_record *, int32_t);
int32_t **probe_union_packed_mutable_slot(struct packed_choice_record *, int32_t);
const int32_t **probe_union_const_slot(struct choice_record *);
void probe_union_store(struct choice_record *, int32_t, int32_t *);
void probe_union_packed_store(struct packed_choice_record *, int32_t, int32_t *);
void probe_union_short_store(struct choice_record *, int16_t *);
void probe_union_raw_store(struct choice_record *, unsigned long long);
unsigned long long probe_union_raw_read(struct choice_record *);
unsigned long long probe_union_layout(int32_t);

static unsigned long cases, checks;

static void check(int condition)
{
    checks++;
    if (!condition) {
        fprintf(stderr, "UNION_MEMBER_FAIL case=%lu check=%lu\n", cases, checks);
        exit(1);
    }
}

int main(void)
{
    const size_t layout[] = {sizeof(union pointer_choice), sizeof(struct choice_record),
        offsetof(struct choice_record, items), offsetof(struct choice_record, suffix),
        sizeof(struct packed_choice_record), offsetof(struct packed_choice_record, items),
        offsetof(struct packed_choice_record, suffix)};
    const unsigned long long patterns[] = {0, 1, 255, 65535, UINT_MAX,
        0x8000000000000000ull, ULLONG_MAX};
    int32_t values[2] = {-417, INT32_MAX};
    int32_t *pointers[] = {NULL, &values[0], &values[1]};
    int16_t short_value = -123;
    for (int32_t index = 0; index < 7; index++)
        check(probe_union_layout(index) == layout[index]);
    for (unsigned fill = 0; fill <= 255; fill++) {
        for (int32_t index = 0; index <= 1; index++) {
            for (size_t p = 0; p < sizeof(pointers) / sizeof(pointers[0]); p++) {
                struct choice_record normal;
                struct packed_choice_record packed;
                unsigned char expected_normal[sizeof(normal)], expected_packed[sizeof(packed)];
                size_t normal_offset = offsetof(struct choice_record, items) + (size_t)index * sizeof(union pointer_choice);
                size_t packed_offset = offsetof(struct packed_choice_record, items) + (size_t)index * sizeof(union pointer_choice);
                cases++;
                memset(&normal, (int)fill, sizeof(normal));
                memset(&packed, (int)fill, sizeof(packed));
                memcpy(expected_normal, &normal, sizeof(normal));
                memcpy(expected_packed, &packed, sizeof(packed));
                memcpy(expected_normal + normal_offset, &pointers[p], sizeof(pointers[p]));
                memcpy(expected_packed + packed_offset, &pointers[p], sizeof(pointers[p]));
                check((void *)probe_union_mutable_slot(&normal, index) == (unsigned char *)&normal + normal_offset);
                check((void *)probe_union_packed_mutable_slot(&packed, index) == (unsigned char *)&packed + packed_offset);
                probe_union_store(&normal, index, pointers[p]);
                probe_union_packed_store(&packed, index, pointers[p]);
                check(memcmp(&normal, expected_normal, sizeof(normal)) == 0);
                check(memcmp(&packed, expected_packed, sizeof(packed)) == 0);
                check((const void *)probe_union_const_slot(&normal) ==
                    (const unsigned char *)&normal + offsetof(struct choice_record, items));
            }
        }
        for (size_t pattern = 0; pattern < sizeof(patterns) / sizeof(patterns[0]); pattern++) {
            struct choice_record value;
            unsigned char expected[sizeof(value)];
            uintptr_t word = (uintptr_t)patterns[pattern];
            cases++;
            memset(&value, (int)fill, sizeof(value));
            memcpy(expected, &value, sizeof(value));
            memcpy(expected + offsetof(struct choice_record, items), &word, sizeof(word));
            probe_union_raw_store(&value, patterns[pattern]);
            check(probe_union_raw_read(&value) == word);
            check(memcmp(&value, expected, sizeof(value)) == 0);
        }
        {
            struct choice_record value;
            unsigned char expected[sizeof(value)];
            int16_t *pointer = &short_value;
            cases++;
            memset(&value, (int)fill, sizeof(value));
            memcpy(expected, &value, sizeof(value));
            memcpy(expected + offsetof(struct choice_record, items), &pointer, sizeof(pointer));
            probe_union_short_store(&value, pointer);
            check(memcmp(&value, expected, sizeof(value)) == 0);
        }
    }
    printf("UNION_MEMBER_PASS cases=%lu checks=%lu\n", cases, checks);
    return 0;
}

/* end of c-union-member-qualifiers.c */
