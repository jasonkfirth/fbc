/*
    FreeBASIC packed structure-return regression
    File: struct-return-packed.c
    Purpose: Return 9..16-byte aggregates through the genuine C ABI.
    Responsibilities: Exact payload bytes for register-return tail checks.
    This file contains no compiler emitter or replacement ABI.
*/
#include <stdint.h>

/* Packed sizes exercise every possible tail of the second return register. */
struct __attribute__((packed)) Packed9 {
    uint64_t head;
    uint8_t tail[1];
};

struct Packed9 make9(void)
{
    struct Packed9 value = {UINT64_C(0x11223344aabbccdd), {128}};
    return value;
}

struct __attribute__((packed)) Packed10 {
    uint64_t head;
    uint8_t tail[2];
};

struct Packed10 make10(void)
{
    struct Packed10 value = {UINT64_C(0x11223344aabbccdd), {128,129}};
    return value;
}

struct __attribute__((packed)) Packed11 {
    uint64_t head;
    uint8_t tail[3];
};

struct Packed11 make11(void)
{
    struct Packed11 value = {UINT64_C(0x11223344aabbccdd), {128,129,130}};
    return value;
}

struct __attribute__((packed)) Packed12 {
    uint64_t head;
    uint8_t tail[4];
};

struct Packed12 make12(void)
{
    struct Packed12 value = {UINT64_C(0x11223344aabbccdd), {128,129,130,131}};
    return value;
}

struct __attribute__((packed)) Packed13 {
    uint64_t head;
    uint8_t tail[5];
};

struct Packed13 make13(void)
{
    struct Packed13 value = {UINT64_C(0x11223344aabbccdd), {128,129,130,131,132}};
    return value;
}

struct __attribute__((packed)) Packed14 {
    uint64_t head;
    uint8_t tail[6];
};

struct Packed14 make14(void)
{
    struct Packed14 value = {UINT64_C(0x11223344aabbccdd), {128,129,130,131,132,133}};
    return value;
}

struct __attribute__((packed)) Packed15 {
    uint64_t head;
    uint8_t tail[7];
};

struct Packed15 make15(void)
{
    struct Packed15 value = {UINT64_C(0x11223344aabbccdd), {128,129,130,131,132,133,134}};
    return value;
}

struct __attribute__((packed)) Packed16 {
    uint64_t head;
    uint8_t tail[8];
};

struct Packed16 make16(void)
{
    struct Packed16 value = {UINT64_C(0x11223344aabbccdd), {128,129,130,131,132,133,134,135}};
    return value;
}

/* end of struct-return-packed.c */
