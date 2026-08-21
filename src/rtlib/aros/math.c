/*
    FreeBASIC runtime library
    -------------------------

    File: aros/math.c

    Purpose:

        Provide accurate double-precision exponential functions on AROS.

    Responsibilities:

        - improve double exp() through a wider AROS long-double routine
        - improve double pow() through a wider AROS long-double routine
        - avoid interposing when long double has no additional precision
        - keep the workaround local to FreeBASIC programs targeting AROS

    This file intentionally does NOT contain:

        - generic FreeBASIC constant-folding policy
        - architecture-specific floating-point baselines
        - replacements for unrelated AROS math entry points

    AROS stdc's double exp() and pow() implementations can miss the correctly
    rounded result by one ULP for ordinary inputs.  On ports with a wider long
    double type, the long-double routines retain enough intermediate precision
    and the final cast supplies the expected double result.  Ports whose long
    double type is identical to double retain the AROS entry points; calling
    expl() or powl() there can alias back to these symbols and recurse.
*/

#include <float.h>
#include <math.h>

/* ------------------------------------------------------------------------- */
/* Double-precision entry points                                             */
/* ------------------------------------------------------------------------- */

#if LDBL_MANT_DIG > DBL_MANT_DIG
	double exp( double value )
	{
		return (double)expl( (long double)value );
	}

	double pow( double base, double exponent )
	{
		return (double)powl( (long double)base, (long double)exponent );
	}
#endif

/* end of aros/math.c */
