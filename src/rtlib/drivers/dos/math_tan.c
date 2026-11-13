/*
 * Project: FreeBASIC DOS runtime
 * File: drivers/dos/math_tan.c
 * Purpose: Evaluate DJGPP's Double tangent at its required precision.
 * Responsibilities: Preserve the caller's x87 control word and Double result.
 * This file intentionally does NOT contain: rounding-mode policy or
 * argument reduction, which remain the caller's and libm's responsibilities.
 */

#include "../../fb.h"
#include <float.h>
#include <math.h>

FBCALL double fb_DosTan( double value )
{
	unsigned int control = _control87( 0, 0 );
	volatile double result;

	/* fdlibm's error compensation assumes 53-bit Double arithmetic. BASIC
	 * leaves the x87 in extended precision, which changes tan(1) by an ulp.
	 * Limit only this call's precision, and store before restoring the caller's
	 * control word. This is current-thread FPU state, not process-wide policy.
	 */
	_control87( PC_53, MCW_PC );
	result = tan( value );
	_control87( control, MCW_PC );
	return result;
}

/* end of drivers/dos/math_tan.c */
