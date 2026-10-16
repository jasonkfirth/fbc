/*
    FreeBASIC classic AmigaOS arithmetic qualification
    ------------------------------------------------
    File: softfloat-multiply.c
    Purpose: Compare native software multiplication with a hardware oracle.
    Responsibilities: Cover rounding, finite boundaries and signed-zero division.
    This file intentionally does NOT contain: a multiplication implementation.

    Reference layout, integers in big-endian byte order:
        0..7: FBMUL001; 8..11: record count.
        Each 36-byte record contains three double bit patterns (left, right,
        result), then three float bit patterns in the same order. NaN payloads
        are unspecified; all other results, including signed zeros, match bits.
*/

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define TEST_CASES 10000U
#define MAX_REFERENCE_CASES 100000U

#ifdef __amigaos__
extern double __muldf3(double, double);
extern float __mulsf3(float, float);
extern double __divdf3(double, double);
extern float __divsf3(float, float);
#endif

static int check_division_signs(void)
{
    unsigned i;
    for (i = 0; i < 4; ++i) {
        uint64_t da = UINT64_C(0x3ff0000000000000), db = UINT64_C(0x7ff0000000000000);
        uint64_t expected, actual_double;
        uint32_t fa = UINT32_C(0x3f800000), fb = UINT32_C(0x7f800000), actual_float;
        double a, b, result;
        float c, d, product;
        if (i & 1) { da |= UINT64_C(0x8000000000000000); fa |= UINT32_C(0x80000000); }
        if (i & 2) { db |= UINT64_C(0x8000000000000000); fb |= UINT32_C(0x80000000); }
        expected = (da ^ db) & UINT64_C(0x8000000000000000);
        memcpy(&a, &da, sizeof(a)); memcpy(&b, &db, sizeof(b));
        memcpy(&c, &fa, sizeof(c)); memcpy(&d, &fb, sizeof(d));
#ifdef __amigaos__
        result = __divdf3(a, b); product = __divsf3(c, d);
#else
        /* Volatile operands keep this host check on the hardware path. */
        { volatile double x = a, y = b; result = x / y; }
        { volatile float x = c, y = d; product = x / y; }
#endif
        memcpy(&actual_double, &result, sizeof(actual_double));
        memcpy(&actual_float, &product, sizeof(actual_float));
        if (actual_double != expected || actual_float != (uint32_t)(expected >> 32)) {
            printf("Signed-zero division mismatch at case %u\n", i);
            return 0;
        }
    }
    printf("8 signed-zero division checks passed\n");
    return 1;
}

static uint64_t get_integer(const unsigned char *bytes, unsigned width)
{
    uint64_t value = 0;
    unsigned i;
    for (i = 0; i < width; ++i) value = (value << 8) | bytes[i];
    return value;
}

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
    unsigned char header[12] = "FBMUL001", record[36];
    uint64_t random = UINT64_C(0x32bfb1584a679e03);
    const unsigned nd = sizeof(doubles) / sizeof(doubles[0]);
    const unsigned nf = sizeof(floats) / sizeof(floats[0]);
    unsigned i;
    FILE *file = fopen(filename, "wb");
    if (file == NULL) return 2;
    put_integer(header + 8, TEST_CASES, 4);
    if (fwrite(header, sizeof(header), 1, file) != 1) { fclose(file); return 2; }
    for (i = 0; i < TEST_CASES; ++i) {
        uint64_t da, db, dr;
        uint32_t fa, fb, fr;
        volatile double left_double, right_double, result_double;
        volatile float left_float, right_float, result_float;
        double d;
        float f;
        random = random * UINT64_C(6364136223846793005) + 1;
        da = i < nd * nd ? doubles[i / nd] : random;
        random = random * UINT64_C(6364136223846793005) + 1;
        db = i < nd * nd ? doubles[i % nd] : random;
        fa = i < nf * nf ? floats[i / nf] : (uint32_t)da;
        fb = i < nf * nf ? floats[i % nf] : (uint32_t)db;
        /* The first case is the luma regression: 116 * 0.71 requires
           renormalization while retaining a nonzero sticky bit. */
        if (i == 0) {
            da = UINT64_C(0x405d000000000000);
            db = UINT64_C(0x3fe6b851eb851eb8);
        }
        memcpy(&d, &da, sizeof(d)); left_double = d;
        memcpy(&d, &db, sizeof(d)); right_double = d;
        memcpy(&f, &fa, sizeof(f)); left_float = f;
        memcpy(&f, &fb, sizeof(f)); right_float = f;
        result_double = left_double * right_double;
        result_float = left_float * right_float;
        d = result_double; f = result_float;
        memcpy(&dr, &d, sizeof(dr)); memcpy(&fr, &f, sizeof(fr));
        put_integer(record, da, 8); put_integer(record + 8, db, 8);
        put_integer(record + 16, dr, 8); put_integer(record + 24, fa, 4);
        put_integer(record + 28, fb, 4); put_integer(record + 32, fr, 4);
        if (fwrite(record, sizeof(record), 1, file) != 1) { fclose(file); return 2; }
    }
    if (fclose(file) != 0) return 2;
    printf("Generated 20000 hardware multiplication results\n");
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
    uint64_t da = get_integer(record, 8), db = get_integer(record + 8, 8), actual_double;
    uint64_t expected_double = get_integer(record + 16, 8);
    uint32_t fa = (uint32_t)get_integer(record + 24, 4);
    uint32_t fb = (uint32_t)get_integer(record + 28, 4), actual_float;
    uint32_t expected_float = (uint32_t)get_integer(record + 32, 4);
    double a, b, result;
    float c, d, product;
    memcpy(&a, &da, sizeof(a)); memcpy(&b, &db, sizeof(b));
    memcpy(&c, &fa, sizeof(c)); memcpy(&d, &fb, sizeof(d));
    result = __muldf3(a, b); product = __mulsf3(c, d);
    memcpy(&actual_double, &result, sizeof(actual_double));
    memcpy(&actual_float, &product, sizeof(actual_float));
    if (actual_double != expected_double &&
        !(is_nan(actual_double, 8) && is_nan(expected_double, 8))) {
        printf("Double multiplication mismatch at case %u: %08lx%08lx != %08lx%08lx\n",
            index, (unsigned long)(actual_double >> 32), (unsigned long)(uint32_t)actual_double,
            (unsigned long)(expected_double >> 32), (unsigned long)(uint32_t)expected_double);
        return 0;
    }
    if (actual_float != expected_float &&
        !(is_nan(actual_float, 4) && is_nan(expected_float, 4))) {
        printf("Float multiplication mismatch at case %u: %08lx != %08lx\n",
            index, (unsigned long)actual_float, (unsigned long)expected_float);
        return 0;
    }
    return 1;
}
#endif

int main(int argc, char **argv)
{
#ifndef __amigaos__
    if (argc != 2) return 2;
    if (!check_division_signs()) return 1;
    return generate_reference(argv[1]);
#else
    unsigned char header[12], record[36];
    uint32_t count, i;
    int extra, failed, closed;
    FILE *file = fopen("multiply-reference.bin", "rb");
    (void)argc; (void)argv;
    if (file == NULL) return 2;
    if (fread(header, sizeof(header), 1, file) != 1 || memcmp(header, "FBMUL001", 8) != 0) {
        fclose(file); return 2;
    }
    count = (uint32_t)get_integer(header + 8, 4);
    if (count == 0 || count > MAX_REFERENCE_CASES) { fclose(file); return 2; }
    for (i = 0; i < count; ++i) {
        if (fread(record, sizeof(record), 1, file) != 1) { fclose(file); return 2; }
        if (!check_record(record, i)) { fclose(file); return 1; }
    }
    extra = fgetc(file); failed = ferror(file); closed = fclose(file);
    if (extra != EOF || failed || closed != 0) return 2;
    if (!check_division_signs()) return 1;
    printf("%lu hardware-oracle multiplication checks passed\n", (unsigned long)count * 2);
    return 0;
#endif
}

/* end of softfloat-multiply.c */
