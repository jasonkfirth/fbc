/*
    Project: FreeBASIC C backend regressions
    File: c-static-float-effects.c
    Purpose: Supply a runtime double and observe its narrowing exception.
    Responsibilities: Own the volatile source and standard fenv calls.
    This file intentionally does NOT access the BASIC static destination.
*/
#include <fenv.h>

/* The first integer above the range represented exactly by a binary32 float. */
static volatile double source_value = 16777217.0;

double fbc_static_float_source(void);
int fbc_static_float_clear(void);
int fbc_static_float_inexact(void);

double fbc_static_float_source(void)
{
    return source_value;
}

int fbc_static_float_clear(void)
{
    return feclearexcept(FE_ALL_EXCEPT);
}

int fbc_static_float_inexact(void)
{
    return (fetestexcept(FE_INEXACT) != 0);
}

/* end of c-static-float-effects.c */
