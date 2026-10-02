/*
    FreeBASIC classic AmigaOS regression tests
    -----------------------------------------

    File: softfloat-convert.c

    Purpose:
        Check software format conversion against a host hardware-FP oracle.

    Responsibilities:
        - compare boundary values and reproducible random bit patterns
        - export reference cases for a native 68020 emulator run
        - validate the same reference data without an FPU on AmigaOS

    This file intentionally does NOT contain:
        - a copy of the software conversion algorithm or approximate checks

    Reference file layout, all integers in big-endian byte order:
        0..7  : ASCII FBSOFT01
        8..11 : record count
        each 24-byte record: double input, expected float, float input,
        expected double (8, 4, 4, and 8 bytes respectively).
    The host computes expected values through volatile hardware conversions.
    The native program only calls the software helpers and compares bits.
*/

#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern float __truncdfsf2(double value);
extern double __extendsfdf2(float value);

#define TEST_CASES 10000U
#define MAX_REFERENCE_CASES 100000U

#ifndef __amigaos__
static void put_integer(unsigned char *bytes, uint64_t value, unsigned width)
{
    while (width != 0) {
        bytes[--width] = (unsigned char)value;
        value >>= 8;
    }
}
#endif

static uint64_t get_integer(const unsigned char *bytes, unsigned width)
{
    uint64_t value = 0;
    unsigned i;
    for (i = 0; i < width; ++i) value = (value << 8) | bytes[i];
    return value;
}

static int check_record(const unsigned char *record)
{
    uint64_t input_double = get_integer(record, 8), actual_double;
    uint32_t input_float = (uint32_t)get_integer(record + 12, 4), actual_float;
    double d, widened;
    float f, narrowed;

    memcpy(&d, &input_double, sizeof(d));
    memcpy(&f, &input_float, sizeof(f));
    narrowed = __truncdfsf2(d);
    widened = __extendsfdf2(f);
    memcpy(&actual_float, &narrowed, sizeof(actual_float));
    memcpy(&actual_double, &widened, sizeof(actual_double));
    return actual_float == get_integer(record + 8, 4) &&
        actual_double == get_integer(record + 16, 8);
}

#ifndef __amigaos__
static int generate_reference(const char *filename)
{
    /* Zero, subnormal boundaries, halfway cases, largest finite values,
       infinities, and both quiet and signalling NaNs precede the random set. */
    static const uint64_t doubles[] = {
        UINT64_C(0), UINT64_C(0x8000000000000000), UINT64_C(1),
        UINT64_C(0x3690000000000000), UINT64_C(0x3690000000000001),
        UINT64_C(0x36a0000000000000), UINT64_C(0x380fffffe0000000),
        UINT64_C(0x3810000000000000), UINT64_C(0x3ff0000010000000),
        UINT64_C(0x3ff0000030000000), UINT64_C(0x47efffffe0000000),
        UINT64_C(0x47effffff0000000), UINT64_C(0x7fefffffffffffff),
        UINT64_C(0x7ff0000000000000), UINT64_C(0xfff0000000000000),
        UINT64_C(0x7ff8000000000000), UINT64_C(0x7ff0000000000001)
    };
    static const uint32_t floats[] = {
        0, UINT32_C(0x80000000), 1, UINT32_C(0x007fffff),
        UINT32_C(0x00800000), UINT32_C(0x3f800000), UINT32_C(0x7f7fffff),
        UINT32_C(0x7f800000), UINT32_C(0xff800000), UINT32_C(0x7fc00000),
        UINT32_C(0x7f800001), UINT32_C(0xffbfffff)
    };
    unsigned char header[12] = "FBSOFT01", record[24];
    uint64_t random = UINT64_C(0xc31a52016bf4a907);
    unsigned i;
    FILE *file = filename != NULL ? fopen(filename, "wb") : NULL;
    if (filename != NULL && file == NULL) return 2;
    put_integer(header + 8, TEST_CASES, 4);
    if (file != NULL && fwrite(header, sizeof(header), 1, file) != 1) return 2;

    for (i = 0; i < TEST_CASES; ++i) {
        uint64_t input_double, expected_double;
        uint32_t input_float, expected_float;
        double d, widened;
        float f, narrowed;
        volatile double host_double;
        volatile float host_float;

        random = random * UINT64_C(6364136223846793005) + 1;
        input_double = i < sizeof(doubles) / sizeof(doubles[0]) ? doubles[i] : random;
        random = random * UINT64_C(6364136223846793005) + 1;
        input_float = i < sizeof(floats) / sizeof(floats[0]) ? floats[i] : (uint32_t)(random >> 32);
        memcpy(&d, &input_double, sizeof(d));
        memcpy(&f, &input_float, sizeof(f));
        host_double = d;
        host_float = f;
        narrowed = (float)host_double;
        widened = (double)host_float;
        memcpy(&expected_float, &narrowed, sizeof(expected_float));
        memcpy(&expected_double, &widened, sizeof(expected_double));
        put_integer(record, input_double, 8);
        put_integer(record + 8, expected_float, 4);
        put_integer(record + 12, input_float, 4);
        put_integer(record + 16, expected_double, 8);
        if (!check_record(record)) {
            fprintf(stderr, "Software conversion mismatch at case %u\n", i);
            if (file != NULL) fclose(file);
            return 1;
        }
        if (file != NULL && fwrite(record, sizeof(record), 1, file) != 1) return 2;
    }
    if (file != NULL && fclose(file) != 0) return 2;
    printf("20000 hardware-oracle conversion checks passed\n");
    return 0;
}
#endif

int main(int argc, char **argv)
{
#ifndef __amigaos__
    if (argc > 2) return 2;
    return generate_reference(argc == 2 ? argv[1] : NULL);
#else
    unsigned char header[12], record[24];
    uint32_t count, i;
    FILE *file = fopen("float-reference.bin", "rb");
    (void)argc;
    (void)argv;
    if (file == NULL) return 2;
    if (fread(header, sizeof(header), 1, file) != 1 ||
        memcmp(header, "FBSOFT01", 8) != 0) { fclose(file); return 2; }
    count = (uint32_t)get_integer(header + 8, 4);
    if (count == 0 || count > MAX_REFERENCE_CASES) { fclose(file); return 2; }
    for (i = 0; i < count; ++i) {
        if (fread(record, sizeof(record), 1, file) != 1) { fclose(file); return 2; }
        if (!check_record(record)) {
            printf("Software conversion mismatch at case %lu\n", (unsigned long)i);
            fclose(file);
            return 1;
        }
    }
    if (fgetc(file) != EOF || fclose(file) != 0) return 2;
    printf("%lu hardware-oracle conversion checks passed\n", (unsigned long)count * 2);
    return 0;
#endif
}

/* end of softfloat-convert.c */
