/*
    Project: FreeBASIC C backend regression controls
    File: c-evaluated-dead-stores-caller.c
    Purpose: Supply runtime operands and inspect the C floating environment.
    Responsibilities: Rounding modes, exception flags and operand call counts.
    This file intentionally does NOT contain: BASIC conversion implementations.
*/
#include <fenv.h>
#include <float.h>
#include <stdint.h>
#include <string.h>

_Static_assert(FLT_RADIX == 2 && DBL_MANT_DIG == 53 && DBL_MAX_EXP == 1024,
               "The signaling-NaN copy control requires binary64 double");
_Static_assert(sizeof(double) == sizeof(uint64_t), "binary64 storage width");

int fp_begin(int mode);
double fp_operand(int index);
int fp_index(void);
int fp_flags(void);
int fp_calls(void);
int fp_mode(void);
int fp_expected(int mode);
void fp_finish(void);

/* This fixture runs serially; all counters belong to its one test process. */
static int calls;
static const int modes[] = { FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO };

int fp_begin(int mode)
{
    if (mode < 0 || mode >= (int)(sizeof(modes) / sizeof(modes[0])))
        return -1;
    calls = 0;
    if (fesetround(modes[mode]) != 0)
        return -1;
    return feclearexcept(FE_ALL_EXCEPT);
}

double fp_operand(int index)
{
    double result;
    /* IEEE binary64 signaling NaN: all exponent bits, payload bit zero set. */
    const uint64_t signaling_nan = UINT64_C(0x7ff0000000000001);
    ++calls;
    switch (index) {
    case 0: return 1.5;
    case 1: return -1.5;
    case 2: return 1.0;
    case 3: return 3.0;
    case 4: return DBL_MAX;
    case 5: return 2.0;
    case 6: return 0.0;
    case 7:
        memcpy(&result, &signaling_nan, sizeof(result));
        return result;
    default: return 0.0;
    }
}

int fp_index(void)
{
    ++calls;
    return 0;
}

int fp_flags(void)
{
    return fetestexcept(FE_ALL_EXCEPT);
}

int fp_calls(void)
{
    return calls;
}

int fp_mode(void)
{
    int mode;
    const int current = fegetround();
    for (mode = 0; mode < (int)(sizeof(modes) / sizeof(modes[0])); ++mode) {
        if (current == modes[mode])
            return mode;
    }
    return -1;
}

int fp_expected(int mode)
{
    /* 1.5 rounds to 2 for nearest/upward and to 1 for downward/toward zero. */
    return (mode == 0 || mode == 2) ? 2 : 1;
}

void fp_finish(void)
{
    (void)fesetround(FE_TONEAREST);
}

/* end of c-evaluated-dead-stores-caller.c */
