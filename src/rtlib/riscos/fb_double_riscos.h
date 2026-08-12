/*
    FreeBASIC RISC OS double-precision representation helpers
    ---------------------------------------------------------

    File: fb_double_riscos.h

    Purpose:

        Convert between the IEEE-754 bit pattern used by FreeBASIC language
        operations and the word-swapped double representation required by the
        RISC OS ARM Procedure Call Standard (APCS).

    Responsibilities:

        - expose a double as its canonical 64-bit IEEE-754 bit pattern
        - construct an APCS double from a canonical 64-bit bit pattern
        - perform all representation changes without violating C aliasing rules

    This file intentionally does NOT contain:

        - floating-point arithmetic
        - byte-order conversion for non-RISC OS targets
        - string encoding or numeric formatting
*/

#ifndef FB_DOUBLE_RISCOS_H
#define FB_DOUBLE_RISCOS_H

/* ------------------------------------------------------------------------- */
/* APCS double conversion                                                    */
/* ------------------------------------------------------------------------- */

static __inline__ uint64_t fb_riscos_DoubleToBits( double value )
{
	uint32_t words[2];

	/*
	    APCS stores the most-significant 32-bit word first, while keeping the
	    bytes within each word little-endian.  A normal uint64_t uses the
	    opposite word order on this little-endian target.
	*/
	memcpy( words, &value, sizeof( words ) );
	return ((uint64_t)words[0] << 32) | (uint64_t)words[1];
}

static __inline__ double fb_riscos_DoubleFromBits( uint64_t bits )
{
	double value;
	uint32_t words[2];

	words[0] = (uint32_t)(bits >> 32);
	words[1] = (uint32_t)bits;
	memcpy( &value, words, sizeof( value ) );
	return value;
}

#endif

/* end of fb_double_riscos.h */
