/*
 * Project FreeBASIC C2FB expression qualification
 * File c2fb-volatile-updates.c
 * Purpose Compare volatile updates with independent native C expressions.
 * Responsibilities Check values, access events, widths and single evaluation.
 * Process execution and complete applications have separate qualification.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned long long word;
typedef word (*update_function)(void *, word, int *);

#define DECLARE_UPDATE(tag, operation) \
    word audit_##tag##_##operation(void *, word, int *); \
    word reference_audit_##tag##_##operation(void *, word, int *);
#define DECLARE_WIDTH(tag) \
    DECLARE_UPDATE(tag, assign) \
    DECLARE_UPDATE(tag, add) \
    DECLARE_UPDATE(tag, xor) \
    DECLARE_UPDATE(tag, preinc) \
    DECLARE_UPDATE(tag, postinc) \
    DECLARE_UPDATE(tag, predec) \
    DECLARE_UPDATE(tag, postdec) \
    DECLARE_UPDATE(tag, reload) \
    DECLARE_UPDATE(tag, sequence)
DECLARE_WIDTH(u8)
DECLARE_WIDTH(u16)
DECLARE_WIDTH(u32)
DECLARE_WIDTH(u64)
DECLARE_WIDTH(s8)
DECLARE_WIDTH(s16)
DECLARE_WIDTH(s32)
DECLARE_WIDTH(s64)

void *audit_ptr_post(void *, int *);
const void *audit_cptr_pre(void *, int *);
void *audit_ptr_add(void *, word, int *);
void *reference_audit_ptr_post(void *, int *);
const void *reference_audit_cptr_pre(void *, int *);
void *reference_audit_ptr_add(void *, word, int *);
void audit_volatile_before_read(const void *, unsigned int);
void audit_volatile_after_write(void *, unsigned int);

enum operation { ASSIGN, ADD, XOR, PREINC, POSTINC, PREDEC, POSTDEC, RELOAD, SEQUENCE };
struct update_case {
    const char *name;
    update_function actual, reference;
    unsigned int width, is_signed;
    enum operation operation;
};

#define CASE(tag, bits, signedness, operation, code) \
    {#tag "_" #operation, audit_##tag##_##operation, reference_audit_##tag##_##operation, bits, signedness, code}
#define WIDTH_CASES(tag, bits, signedness) \
    CASE(tag, bits, signedness, assign, ASSIGN), \
    CASE(tag, bits, signedness, add, ADD), \
    CASE(tag, bits, signedness, xor, XOR), \
    CASE(tag, bits, signedness, preinc, PREINC), \
    CASE(tag, bits, signedness, postinc, POSTINC), \
    CASE(tag, bits, signedness, predec, PREDEC), \
    CASE(tag, bits, signedness, postdec, POSTDEC), \
    CASE(tag, bits, signedness, reload, RELOAD), \
    CASE(tag, bits, signedness, sequence, SEQUENCE)

static const struct update_case functions[] = {
    WIDTH_CASES(u8, 8, 0), WIDTH_CASES(u16, 16, 0),
    WIDTH_CASES(u32, 32, 0), WIDTH_CASES(u64, 64, 0),
    WIDTH_CASES(s8, 8, 1), WIDTH_CASES(s16, 16, 1),
    WIDTH_CASES(s32, 32, 1), WIDTH_CASES(s64, 64, 1)
};

union slot {
    unsigned char u8;
    unsigned short u16;
    unsigned int u32;
    word u64;
    signed char s8;
    short s16;
    int s32;
    long long s64;
    int *pointer;
    const int *const_pointer;
};

struct guarded_slot { word before; union slot value; word after; };
_Static_assert(sizeof(word) == 8 && sizeof(void *) == 8, "64-bit observer required");

static unsigned long cases, checks;
static const char *active_name;
static void *active_slot;
static unsigned int active_width, reads, writes;
static word noise, last_written;

static void require_at(int condition, int line)
{
    checks++;
    if (!condition) {
        fprintf(stderr, "VOLATILE_UPDATE_FAIL case=%lu check=%lu function=%s line=%d\n",
            cases, checks, active_name, line);
        exit(1);
    }
}

#define require(condition) require_at((condition), __LINE__)

static word slot_bits(const void *slot, unsigned int width)
{
    word value = 0;
    memcpy(&value, slot, width / 8);
    return value;
}

void audit_volatile_before_read(const void *slot, unsigned int width)
{
    require(slot == active_slot && width == active_width);
    reads++;
}

void audit_volatile_after_write(void *slot, unsigned int width)
{
    require(slot == active_slot && width == active_width);
    writes++;
    last_written = slot_bits(slot, width);
    /* Changing the slot after the physical store exposes an accidental
       reload when an expression should return its already converted value. */
    memcpy(slot, &noise, width / 8);
}

static word width_mask(unsigned int width)
{
    return width == 64 ? ~0ULL : (1ULL << width) - 1;
}

static void *prepare_slot(union slot *slot, unsigned int width, unsigned int is_signed, word value)
{
    if (is_signed) {
        switch (width) {
        case 8: slot->s8 = (signed char)value; return &slot->s8;
        case 16: slot->s16 = (short)value; return &slot->s16;
        case 32: slot->s32 = (int)value; return &slot->s32;
        default: slot->s64 = (long long)value; return &slot->s64;
        }
    }
    switch (width) {
    case 8: slot->u8 = (unsigned char)value; return &slot->u8;
    case 16: slot->u16 = (unsigned short)value; return &slot->u16;
    case 32: slot->u32 = (unsigned int)value; return &slot->u32;
    default: slot->u64 = value; return &slot->u64;
    }
}

static void check_integer(const struct update_case *function, word initial, word input)
{
    struct guarded_slot actual, reference;
    int actual_effects = 0, reference_effects = 0;
    memset(&actual, 0xa5, sizeof(actual));
    actual.before = 0x123456789abcdef0ULL;
    actual.after = 0xfedcba9876543210ULL;
    active_name = function->name;
    active_slot = prepare_slot(&actual.value, function->width, function->is_signed, initial);
    memcpy(&reference, &actual, sizeof(actual));
    word expected = function->reference(&reference.value, input, &reference_effects);
#if AUDIT_TRACKED
    word expected_stored = slot_bits(&reference.value, function->width);
#endif
    active_width = function->width;
    reads = writes = 0;
    noise = 0xd39a82e571b6c40fULL & width_mask(active_width);
    word returned = function->actual(active_slot, input, &actual_effects);
#if AUDIT_TRACKED
    require(reads == (function->operation == ASSIGN || function->operation == SEQUENCE ? 0u : 1u));
    require(writes == (function->operation == SEQUENCE ? 2u : 1u));
    require(last_written == expected_stored);
    memcpy(&reference.value, &noise, active_width / 8);
    if (function->operation == RELOAD) {
        expected = noise;
        if (function->is_signed && active_width < 64 && (noise & (1ULL << (active_width - 1))))
            expected |= ~width_mask(active_width);
    }
#endif
    require(returned == expected);
    require(actual_effects == reference_effects);
    require(actual_effects == (function->operation >= PREINC && function->operation <= POSTDEC ? 1 : 101));
    require(memcmp(&actual, &reference, sizeof(actual)) == 0);
    cases++;
}

static void check_pointer(unsigned int operation, unsigned int index, word step)
{
    int targets[8] = {0};
    struct guarded_slot actual, reference;
    int actual_effects = 0, reference_effects = 0;
    const void *returned, *expected;
    memset(&actual, 0xa5, sizeof(actual));
    actual.before = 0x123456789abcdef0ULL;
    actual.after = 0xfedcba9876543210ULL;
    active_name = operation == 1 ? "const_pointer_preincrement" : "pointer_update";
    if (operation == 1) {
        actual.value.const_pointer = &targets[index];
        active_slot = &actual.value.const_pointer;
    } else {
        actual.value.pointer = &targets[index];
        active_slot = &actual.value.pointer;
    }
    memcpy(&reference, &actual, sizeof(actual));
    if (operation == 0)
        expected = reference_audit_ptr_post(&reference.value.pointer, &reference_effects);
    else if (operation == 1)
        expected = reference_audit_cptr_pre(&reference.value.const_pointer, &reference_effects);
    else
        expected = reference_audit_ptr_add(&reference.value.pointer, step, &reference_effects);
    active_width = 64;
    reads = writes = 0;
    const int *noise_pointer = &targets[6];
    memcpy(&noise, &noise_pointer, sizeof(noise));
    if (operation == 0)
        returned = audit_ptr_post(active_slot, &actual_effects);
    else if (operation == 1)
        returned = audit_cptr_pre(active_slot, &actual_effects);
    else
        returned = audit_ptr_add(active_slot, step, &actual_effects);
#if AUDIT_TRACKED
    require(reads == 1 && writes == 1);
    require(last_written == slot_bits(&reference.value, 64));
    memcpy(&reference.value, &noise, sizeof(noise));
#endif
    require(returned == expected);
    require(actual_effects == reference_effects && actual_effects == (operation == 2 ? 101 : 1));
    require(memcmp(&actual, &reference, sizeof(actual)) == 0);
    cases++;
}

int main(void)
{
    static const word values[] = {
        0, 1, 0x7f, 0xff, 0x100, 0x7fff, 0xffff, 0x10000,
        0x7fffffff, 0xffffffff, 0x100000000ULL, 0x8000000000000000ULL, ~0ULL
    };
    for (unsigned int function = 0; function < sizeof(functions) / sizeof(functions[0]); function++) {
        for (unsigned int initial = 0; initial < sizeof(values) / sizeof(values[0]); initial++) {
            for (unsigned int input = 0; input < sizeof(values) / sizeof(values[0]); input++)
                check_integer(&functions[function], values[initial], values[input]);
        }
    }
    for (unsigned int operation = 0; operation < 3; operation++) {
        for (unsigned int index = 1; index < 4; index++) {
            for (word step = 0; step < 2; step++)
                check_pointer(operation, index, step);
        }
    }
    printf("VOLATILE_UPDATE_PASS cases=%lu checks=%lu\n", cases, checks);
    return 0;
}
/* end of c2fb-volatile-updates.c */
