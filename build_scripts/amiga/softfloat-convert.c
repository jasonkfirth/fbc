/*
    FreeBASIC classic AmigaOS toolchain support
    ------------------------------------------

    File: softfloat-convert.c

    Purpose:
        Convert IEEE binary32 and binary64 values without an FPU or OS library.

    Responsibilities:
        - preserve signs, infinities, NaNs, and subnormal values
        - round narrowing conversions to nearest with ties to even
        - provide the GCC soft-float conversion calling convention

    This file intentionally does NOT contain:
        - floating arithmetic, exceptions, or BASIC numeric conversion policy

    The pinned SDK's conversion routines do not handle all exponent limits.
    Integer operations make those limits explicit and avoid recursively
    invoking the floating conversion helpers implemented here.

    IEEE layouts:
        binary32: sign bit 31, exponent bits 30..23, fraction bits 22..0
        binary64: sign bit 63, exponent bits 62..52, fraction bits 51..0
    The exponent biases are 127 and 1023 respectively. Subnormal values
    have exponent zero and omit the normal implicit leading significand bit.
*/

#include <stdint.h>
#include <string.h>

double __extendsfdf2(float value)
{
    uint32_t source, fraction, exponent;
    uint64_t result, significand;
    double converted;

    memcpy(&source, &value, sizeof(source));
    fraction = source & UINT32_C(0x007fffff);
    exponent = (source >> 23) & 255U;
    result = (uint64_t)(source >> 31) << 63;
    if (exponent == 255U) {
        result |= UINT64_C(0x7ff0000000000000) | ((uint64_t)fraction << 29);
        if (fraction != 0) result |= UINT64_C(0x0008000000000000);
    } else if (exponent != 0) {
        result |= (uint64_t)(exponent + 896U) << 52;
        result |= (uint64_t)fraction << 29;
    } else if (fraction != 0) {
        /* The smallest normal float has double exponent 897. Normalize a
           subnormal fraction before assigning its smaller double exponent. */
        exponent = 897U;
        significand = fraction;
        while ((significand & UINT64_C(0x00800000)) == 0) {
            significand <<= 1;
            --exponent;
        }
        result |= (uint64_t)exponent << 52;
        result |= (significand & UINT64_C(0x007fffff)) << 29;
    }
    memcpy(&converted, &result, sizeof(converted));
    return converted;
}

float __truncdfsf2(double value)
{
    uint64_t source, fraction, significand, remainder, half, rounded;
    uint32_t result, exponent;
    int target_exponent, shift;
    float converted;

    memcpy(&source, &value, sizeof(source));
    fraction = source & UINT64_C(0x000fffffffffffff);
    exponent = (uint32_t)((source >> 52) & 2047U);
    result = (uint32_t)(source >> 32) & UINT32_C(0x80000000);
    if (exponent == 2047U) {
        result |= UINT32_C(0x7f800000) | (uint32_t)(fraction >> 29);
        if (fraction != 0) result |= UINT32_C(0x00400000);
    } else if (exponent != 0) {
        target_exponent = (int)exponent - 896;
        if (target_exponent >= 255) {
            result |= UINT32_C(0x7f800000);
        } else if (target_exponent >= -23) {
            significand = fraction | UINT64_C(0x0010000000000000);
            shift = target_exponent > 0 ? 29 : 30 - target_exponent;
            rounded = significand >> shift;
            remainder = significand & ((UINT64_C(1) << shift) - 1);
            half = UINT64_C(1) << (shift - 1);
            if (remainder > half || (remainder == half && (rounded & 1)))
                ++rounded;
            if (target_exponent > 0) {
                if (rounded == UINT64_C(0x01000000)) {
                    rounded >>= 1;
                    ++target_exponent;
                }
                result |= (uint32_t)target_exponent << 23;
                result |= (uint32_t)rounded & UINT32_C(0x007fffff);
            } else {
                /* Rounding the largest subnormal can reach the smallest
                   normal float, whose complete encoding is 0x00800000. */
                result |= (uint32_t)rounded;
            }
        }
    }
    memcpy(&converted, &result, sizeof(converted));
    return converted;
}

/* end of softfloat-convert.c */
