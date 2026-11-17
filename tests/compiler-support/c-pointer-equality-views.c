/*
    Project: FreeBASIC C backend regression
    File: c-pointer-equality-views.c
    Purpose: Observe address equality with independent native integer references.
    Responsibilities: Check full-width addresses, typed views and single evaluation.
    The BASIC fixture supplies all pointer comparisons under test.
*/
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct first_record;
struct second_record;
#if UINTPTR_MAX == UINT64_MAX
typedef unsigned long long fb_unsigned_word;
#else
typedef unsigned int fb_unsigned_word;
#endif
union pointer_slots {
    const int32_t *read_only_value;
    int32_t *write_value;
};

int32_t probe_compare_nested(const int32_t **, int32_t **);
int32_t probe_unequal_nested(const int32_t **, int32_t **);
int32_t probe_compare_triple(const int32_t ***, int32_t ***);
int32_t probe_compare_const_slot(int32_t *const *, int32_t **);
int32_t probe_compare_records(struct first_record *, struct second_record *);
int32_t probe_compare_elements(const int32_t *, int32_t *);
int32_t probe_compare_erased(void *, const int32_t **);
int32_t probe_compare_evaluated(void *, void *, int32_t);
int32_t probe_compare_procedures(fb_unsigned_word, fb_unsigned_word);
int32_t probe_compare_members(union pointer_slots *, union pointer_slots *);

static unsigned long cases, checks;

static void require(int condition)
{
    checks++;
    if (!condition) {
        fprintf(stderr, "POINTER_EQUALITY_FAIL case=%lu check=%lu\n", cases, checks);
        exit(1);
    }
}

int main(void)
{
    int32_t objects[2] = {17, 23};
    union pointer_slots slots[2] = {{NULL}, {NULL}};
    uintptr_t values[24] = {0, 1, 2, 255, 256, 65535, 65536,
        UINT32_C(0x7fffffff), UINT32_C(0x80000000), UINT32_MAX,
        (uintptr_t)UINT64_C(0x100000000), (uintptr_t)UINT64_C(0x7fffffffffffffff),
        (uintptr_t)UINT64_C(0x8000000000000000), UINTPTR_MAX,
        (uintptr_t)&objects[0], (uintptr_t)&objects[1],
        (uintptr_t)&slots[0], (uintptr_t)&slots[1],
        3, 15, 16, 31, 32, 4096};
    for (size_t first = 0; first < 24; first++) {
        for (size_t second = 0; second < 24; second++) {
            uintptr_t lhs = values[first], rhs = values[second];
            int32_t equal = lhs == rhs ? -1 : 0;
            cases++;
            require(probe_compare_nested((const int32_t **)lhs, (int32_t **)rhs) == equal);
            require(probe_unequal_nested((const int32_t **)lhs, (int32_t **)rhs) == ~equal);
            require(probe_compare_triple((const int32_t ***)lhs, (int32_t ***)rhs) == equal);
            require(probe_compare_const_slot((int32_t *const *)lhs, (int32_t **)rhs) == equal);
            require(probe_compare_records((struct first_record *)lhs, (struct second_record *)rhs) == equal);
            require(probe_compare_elements((const int32_t *)lhs, (int32_t *)rhs) == equal);
            require(probe_compare_erased((void *)lhs, (const int32_t **)rhs) == equal);
            require(probe_compare_evaluated((void *)lhs, (void *)rhs, 0) == equal);
            require(probe_compare_evaluated((void *)lhs, (void *)rhs, 1) == ~equal);
            require(probe_compare_procedures(lhs, rhs) == equal);
        }
    }
    for (size_t first = 0; first < 2; first++) {
        for (size_t second = 0; second < 2; second++) {
            cases++;
            require(probe_compare_members(&slots[first], &slots[second]) == (first == second ? -1 : 0));
        }
    }
    printf("POINTER_EQUALITY_PASS cases=%lu checks=%lu\n", cases, checks);
    return 0;
}

/* end of c-pointer-equality-views.c */
