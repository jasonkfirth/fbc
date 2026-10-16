/*
    FreeBASIC classic AmigaOS arithmetic qualification
    ------------------------------------------------
    File: softfloat-arithmetic.c
    Purpose: Compare native addition, subtraction and division with hardware.
    Responsibilities: Cover both operand signs, rounding and finite/nonfinite boundaries.
    This file intentionally does NOT contain: an arithmetic implementation.

    Reference layout, integers in big-endian byte order:
        0..7: FBOPS001; 8..11: record count.
        Each 40-byte record starts with a four-byte operation (0 add, 1 subtract,
        2 divide), then three double bit patterns (left, right,
        result), then three float bit patterns in the same order. NaN payloads
        are unspecified; all other results, including signed zeros, match bits.
*/

#include <float.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define TEST_CASES 10000U
#define OPERATIONS 3U

#ifdef __amigaos__
extern double __adddf3(double, double);
extern float __addsf3(float, float);
extern double __subdf3(double, double);
extern float __subsf3(float, float);
extern double __divdf3(double, double);
extern float __divsf3(float, float);
#endif

#ifdef __amigaos__
static uint64_t get_integer(const unsigned char *bytes, unsigned width)
{
    uint64_t value = 0;
    unsigned i;
    for (i = 0; i < width; ++i) value = (value << 8) | bytes[i];
    return value;
}
#endif

#ifndef __amigaos__
static void put_integer(unsigned char *bytes, uint64_t value, unsigned width)
{
    while (width != 0) {
        bytes[--width] = (unsigned char)value;
        value >>= 8;
    }
}

static int generate_reference(const char *filename)
{
    static const uint64_t doubles[] = {
        0, UINT64_C(0x8000000000000000), 1, UINT64_C(0x000fffffffffffff),
        UINT64_C(0x0010000000000000), UINT64_C(0x3fefffffffffffff),
        UINT64_C(0x3ff0000000000000), UINT64_C(0x3ff0000000000001),
        UINT64_C(0x3ff8000000000000), UINT64_C(0x4000000000000000),
        UINT64_C(0x405d000000000000), UINT64_C(0x3fe6b851eb851eb8),
        UINT64_C(0x7fefffffffffffff), UINT64_C(0x7ff0000000000000),
        UINT64_C(0xfff0000000000000), UINT64_C(0x7ff8000000000000),
        UINT64_C(0x7ff0000000000001)
    };
    static const uint32_t floats[] = {
        0, UINT32_C(0x80000000), 1, UINT32_C(0x007fffff),
        UINT32_C(0x00800000), UINT32_C(0x3f7fffff), UINT32_C(0x3f800000),
        UINT32_C(0x3f800001), UINT32_C(0x3fc00000), UINT32_C(0x40000000),
        UINT32_C(0x7f7fffff), UINT32_C(0x7f800000), UINT32_C(0xff800000),
        UINT32_C(0x7fc00000), UINT32_C(0x7f800001)
    };
    unsigned char header[12] = "FBOPS001", record[40];
    uint64_t random = UINT64_C(0x32bfb1584a679e03);
    const unsigned nd = sizeof(doubles) / sizeof(doubles[0]);
    const unsigned nf = sizeof(floats) / sizeof(floats[0]);
    unsigned i;
    FILE *file = fopen(filename, "wb");
    if (file == NULL) return 2;
    put_integer(header + 8, TEST_CASES * OPERATIONS, 4);
    if (fwrite(header, sizeof(header), 1, file) != 1) { fclose(file); return 2; }
    for (i = 0; i < TEST_CASES * OPERATIONS; ++i) {
        unsigned operation = i / TEST_CASES, sample = i % TEST_CASES;
        uint64_t da, db, dr;
        uint32_t fa, fb, fr;
        volatile double left_double, right_double, result_double;
        volatile float left_float, right_float, result_float;
        double d;
        float f;
        random = random * UINT64_C(6364136223846793005) + 1;
        da = sample < nd * nd * 4 ? doubles[(sample / 4) / nd] : random;
        random = random * UINT64_C(6364136223846793005) + 1;
        db = sample < nd * nd * 4 ? doubles[(sample / 4) % nd] : random;
        fa = sample < nf * nf * 4 ? floats[(sample / 4) / nf] : (uint32_t)da;
        fb = sample < nf * nf * 4 ? floats[(sample / 4) % nf] : (uint32_t)db;
        /* Cross every fixed magnitude with both operand signs. This also
           checks exact cancellation, signed zeros and negative subnormals. */
        if (sample < nd * nd * 4) {
            if (sample & 1) da ^= UINT64_C(0x8000000000000000);
            if (sample & 2) db ^= UINT64_C(0x8000000000000000);
        }
        if (sample < nf * nf * 4) {
            if (sample & 1) fa ^= UINT32_C(0x80000000);
            if (sample & 2) fb ^= UINT32_C(0x80000000);
        }
        memcpy(&d, &da, sizeof(d)); left_double = d;
        memcpy(&d, &db, sizeof(d)); right_double = d;
        memcpy(&f, &fa, sizeof(f)); left_float = f;
        memcpy(&f, &fb, sizeof(f)); right_float = f;
        if (operation == 0) {
            result_double = left_double + right_double;
            result_float = left_float + right_float;
        } else if (operation == 1) {
            result_double = left_double - right_double;
            result_float = left_float - right_float;
        } else {
            result_double = left_double / right_double;
            result_float = left_float / right_float;
        }
        d = result_double; f = result_float;
        memcpy(&dr, &d, sizeof(dr)); memcpy(&fr, &f, sizeof(fr));
        put_integer(record, operation, 4);
        put_integer(record + 4, da, 8); put_integer(record + 12, db, 8);
        put_integer(record + 20, dr, 8); put_integer(record + 28, fa, 4);
        put_integer(record + 32, fb, 4); put_integer(record + 36, fr, 4);
        if (fwrite(record, sizeof(record), 1, file) != 1) { fclose(file); return 2; }
    }
    if (fclose(file) != 0) return 2;
    printf("Generated 60000 hardware arithmetic results\n");
    return 0;
}
#else
static int is_nan(uint64_t bits, unsigned width)
{
    if (width == 8)
        return (bits & UINT64_C(0x7ff0000000000000)) == UINT64_C(0x7ff0000000000000) &&
            (bits & UINT64_C(0x000fffffffffffff)) != 0;
    return (bits & UINT32_C(0x7f800000)) == UINT32_C(0x7f800000) &&
        (bits & UINT32_C(0x007fffff)) != 0;
}

static int check_record(const unsigned char *record, unsigned index)
{
    unsigned operation = (unsigned)get_integer(record, 4);
    uint64_t da = get_integer(record + 4, 8), db = get_integer(record + 12, 8), actual_double;
    uint64_t expected_double = get_integer(record + 20, 8);
    uint32_t fa = (uint32_t)get_integer(record + 28, 4);
    uint32_t fb = (uint32_t)get_integer(record + 32, 4), actual_float;
    uint32_t expected_float = (uint32_t)get_integer(record + 36, 4);
    double a, b, result;
    float c, d, float_result;
    if (operation != index / TEST_CASES) {
        printf("Unexpected operation order at case %u\n", index);
        return 0;
    }
    memcpy(&a, &da, sizeof(a)); memcpy(&b, &db, sizeof(b));
    memcpy(&c, &fa, sizeof(c)); memcpy(&d, &fb, sizeof(d));
    if (operation == 0) {
        result = __adddf3(a, b); float_result = __addsf3(c, d);
    } else if (operation == 1) {
        result = __subdf3(a, b); float_result = __subsf3(c, d);
    } else if (operation == 2) {
        result = __divdf3(a, b); float_result = __divsf3(c, d);
    } else {
        printf("Invalid arithmetic operation at case %u\n", index);
        return 0;
    }
    memcpy(&actual_double, &result, sizeof(actual_double));
    memcpy(&actual_float, &float_result, sizeof(actual_float));
    if (actual_double != expected_double &&
        !(is_nan(actual_double, 8) && is_nan(expected_double, 8))) {
        printf("Double arithmetic mismatch at case %u operation %u: %08lx%08lx != %08lx%08lx\n",
            index, operation, (unsigned long)(actual_double >> 32), (unsigned long)(uint32_t)actual_double,
            (unsigned long)(expected_double >> 32), (unsigned long)(uint32_t)expected_double);
        return 0;
    }
    if (actual_float != expected_float &&
        !(is_nan(actual_float, 4) && is_nan(expected_float, 4))) {
        printf("Float arithmetic mismatch at case %u operation %u: %08lx != %08lx\n",
            index, operation, (unsigned long)actual_float, (unsigned long)expected_float);
        return 0;
    }
    return 1;
}
#endif

int main(int argc, char **argv)
{
    if (sizeof(double) != 8 || sizeof(float) != 4 || FLT_RADIX != 2 ||
        DBL_MANT_DIG != 53 || FLT_MANT_DIG != 24) return 2;
#ifndef __amigaos__
    if (argc != 2) return 2;
    return generate_reference(argv[1]);
#else
    unsigned char header[12], record[40];
    uint32_t count, i;
    int extra, failed, closed;
    FILE *file = fopen("arithmetic-reference.bin", "rb");
    (void)argc; (void)argv;
    if (file == NULL) return 2;
    if (fread(header, sizeof(header), 1, file) != 1 || memcmp(header, "FBOPS001", 8) != 0) {
        fclose(file); return 2;
    }
    count = (uint32_t)get_integer(header + 8, 4);
    if (count != TEST_CASES * OPERATIONS) { fclose(file); return 2; }
    for (i = 0; i < count; ++i) {
        if (fread(record, sizeof(record), 1, file) != 1) { fclose(file); return 2; }
        if (!check_record(record, i)) { fclose(file); return 1; }
    }
    extra = fgetc(file); failed = ferror(file); closed = fclose(file);
    if (extra != EOF || failed || closed != 0) return 2;
    printf("%lu hardware-oracle arithmetic checks passed\n", (unsigned long)count * 2);
    return 0;
#endif
}

/* end of softfloat-arithmetic.c */
