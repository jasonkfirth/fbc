/*
    Project: FreeBASIC C backend regression controls
    File: c-conversion-results-caller.c
    Purpose: Supply runtime conversion operands and inspect floating state.
    Responsibilities: Four rounding modes, independent expected integers,
    operand counts and exact floating values near native 128-bit limits.
    This file intentionally does NOT contain: BASIC conversion implementations.
    The fixture runs serially in one process.
*/
#include <fenv.h>

int fpcr_begin(int mode);
double fpcr_double(int index);
float fpcr_single(int index);
double fpcr_large_double(int index);
float fpcr_large_single(int index);
int fpcr_expected(int mode, int index);
int fpcr_calls(void);
int fpcr_flags(void);
int fpcr_mode(void);
void fpcr_finish(void);

static int calls;
static const int modes[] = { FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO };
static const double values[] = {
    0.0, 0.5, 1.5, 2.5, 3.5, 12.75, 1.0, -0.0,
    -0.5, -1.5, -2.5, -3.5, -12.75, -1.0, 17.0, -17.0
};
static const int expected[4][16] = {
    { 0, 0, 2, 2, 4, 13, 1, 0, 0, -2, -2, -4, -13, -1, 17, -17 },
    { 0, 0, 1, 2, 3, 12, 1, 0, -1, -2, -3, -4, -13, -1, 17, -17 },
    { 0, 1, 2, 3, 4, 13, 1, 0, 0, -1, -2, -3, -12, -1, 17, -17 },
    { 0, 0, 1, 2, 3, 12, 1, 0, 0, -1, -2, -3, -12, -1, 17, -17 }
};

int fpcr_begin(int mode)
{
    if (mode < 0 || mode >= 4) return -1;
    calls = 0;
    if (fesetround(modes[mode]) != 0) return -1;
    return feclearexcept(FE_ALL_EXCEPT);
}

double fpcr_double(int index)
{
    ++calls;
    if (index < 0 || index >= 16) return 0.0;
    return values[index];
}

float fpcr_single(int index)
{
    return (float)fpcr_double(index);
}

double fpcr_large_double(int index)
{
    /* The values below each upper limit are its immediate binary64 predecessor. */
    static const double large[] = {
        -0x1p+127, 0x1.fffffffffffffp+126, 0x1.fffffffffffffp+127,
        -0x1p+100, 0x1p+100
    };
    ++calls;
    if (index < 0 || index >= 5) return 0.0;
    return large[index];
}

float fpcr_large_single(int index)
{
    /* SINGLE has 24 significant bits, so these gaps are wider than for DOUBLE. */
    static const float large[] = {
        -0x1p+127f, 0x1.fffffep+126f, 0x1.fffffep+127f,
        -0x1p+100f, 0x1p+100f
    };
    ++calls;
    if (index < 0 || index >= 5) return 0.0f;
    return large[index];
}

int fpcr_expected(int mode, int index)
{
    if (mode < 0 || mode >= 4 || index < 0 || index >= 16) return 0;
    return expected[mode][index];
}

int fpcr_calls(void) { return calls; }
int fpcr_flags(void) { return fetestexcept(FE_ALL_EXCEPT); }

int fpcr_mode(void)
{
    const int current = fegetround();
    int mode;
    for (mode = 0; mode < 4; ++mode) {
        if (current == modes[mode]) return mode;
    }
    return -1;
}

void fpcr_finish(void) { (void)fesetround(FE_TONEAREST); }

/* end of c-conversion-results-caller.c */
