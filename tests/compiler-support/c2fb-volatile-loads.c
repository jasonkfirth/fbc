/*
 * Project FreeBASIC C2FB qualification
 * File c2fb-volatile-loads.c
 * Purpose Observe translated volatile read helpers at each supported width.
 * Responsibilities Check fresh values, pointer identity and surrounding bytes.
 * Native register-allocation and complete application behavior have separate tests.
 */
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned char audit_read8(const unsigned char *);
unsigned short audit_read16(const unsigned short *);
unsigned int audit_read32(const unsigned int *);
unsigned long long audit_read64(const unsigned long long *);
void *audit_read_pointer(void * const *);
void audit_write8(unsigned char *, unsigned char);
void audit_write16(unsigned short *, unsigned short);
void audit_write32(unsigned int *, unsigned int);
void audit_write64(unsigned long long *, unsigned long long);
void audit_write_pointer(void **, void *);

struct slots {
    unsigned char value8;
    unsigned short value16;
    unsigned int value32;
    unsigned long long value64;
    void *pointer;
};

struct guarded_slots {
    uint64_t before;
    struct slots values;
    uint64_t after;
};

_Static_assert(CHAR_BIT == 8 && sizeof(unsigned short) == 2, "8/16-bit slots required");
_Static_assert(sizeof(unsigned int) == 4 && sizeof(unsigned long long) == 8, "32/64-bit slots required");
_Static_assert(sizeof(void *) == 8 && sizeof(struct slots) == 24, "64-bit fixture layout required");
_Static_assert(offsetof(struct slots, pointer) == 16, "pointer slot offset required");

static unsigned long cases, checks;

static void require_at(int condition, int line)
{
    checks++;
    if (!condition) {
        fprintf(stderr, "VOLATILE_LOAD_FAIL case=%lu check=%lu line=%d\n", cases, checks, line);
        exit(1);
    }
}

#define require(condition) require_at((condition), __LINE__)

static void read_values(const struct guarded_slots *record)
{
    struct guarded_slots snapshot;
    memcpy(&snapshot, record, sizeof(snapshot));
    require(audit_read8(&record->values.value8) == record->values.value8);
    require(audit_read16(&record->values.value16) == record->values.value16);
    require(audit_read32(&record->values.value32) == record->values.value32);
    require(audit_read64(&record->values.value64) == record->values.value64);
    require(audit_read_pointer(&record->values.pointer) == record->values.pointer);
    require(memcmp(record, &snapshot, sizeof(snapshot)) == 0);
}

static void check_values(unsigned long long initial, unsigned long long next, void *pointer)
{
    struct guarded_slots actual, reference;
    memset(&reference, 0xa5, sizeof(reference));
    reference.before = UINT64_C(0x123456789abcdef0);
    reference.after = UINT64_C(0xfedcba9876543210);
    reference.values.value8 = (unsigned char)initial;
    reference.values.value16 = (unsigned short)initial;
    reference.values.value32 = (unsigned int)initial;
    reference.values.value64 = initial;
    reference.values.pointer = pointer;
    memcpy(&actual, &reference, sizeof(actual));
    read_values(&actual);

    /* Repeated writes distinguish new reads from reused prior values. A
       full byte comparison also checks padding and both boundary guards. */
    for (unsigned int phase = 0; phase < 4; phase++) {
        unsigned long long value = next ^ (UINT64_C(0x9e3779b97f4a7c15) * phase);
        audit_write8(&actual.values.value8, (unsigned char)value);
        reference.values.value8 = (unsigned char)value;
        require(memcmp(&actual, &reference, sizeof(actual)) == 0);
        read_values(&actual);
        audit_write16(&actual.values.value16, (unsigned short)value);
        reference.values.value16 = (unsigned short)value;
        require(memcmp(&actual, &reference, sizeof(actual)) == 0);
        read_values(&actual);
        audit_write32(&actual.values.value32, (unsigned int)value);
        reference.values.value32 = (unsigned int)value;
        require(memcmp(&actual, &reference, sizeof(actual)) == 0);
        read_values(&actual);
        audit_write64(&actual.values.value64, value);
        reference.values.value64 = value;
        require(memcmp(&actual, &reference, sizeof(actual)) == 0);
        read_values(&actual);
        audit_write_pointer(&actual.values.pointer, phase & 1u ? NULL : pointer);
        reference.values.pointer = phase & 1u ? NULL : pointer;
        require(memcmp(&actual, &reference, sizeof(actual)) == 0);
        read_values(&actual);
    }
    cases++;
}

int main(void)
{
    static const unsigned long long values[] = {
        0, 1, 0x7f, 0xff, 0x100, 0x7fff, 0xffff, 0x10000,
        0x7fffffff, 0xffffffff, UINT64_C(0x100000000),
        UINT64_C(0x8000000000000000), ULLONG_MAX
    };
    unsigned int first = 17, second = 23;
    void *pointers[] = {NULL, &first, &second, (unsigned char *)&first + 1};
    for (unsigned int initial = 0; initial < sizeof(values) / sizeof(values[0]); initial++) {
        for (unsigned int next = 0; next < sizeof(values) / sizeof(values[0]); next++) {
            for (unsigned int pointer = 0; pointer < sizeof(pointers) / sizeof(pointers[0]); pointer++)
                check_values(values[initial], values[next], pointers[pointer]);
        }
    }
    printf("VOLATILE_LOAD_PASS cases=%lu checks=%lu\n", cases, checks);
    return 0;
}
/* end of c2fb-volatile-loads.c */
