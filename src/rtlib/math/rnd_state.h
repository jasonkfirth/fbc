/*
 * Project: FreeBASIC runtime library
 * File: math/rnd_state.h
 * Purpose: Fill the initial word vector used by the RND providers.
 * Responsibilities: Preserve the FAST32 recurrence and caller-owned bounds.
 * This file intentionally does NOT contain: global state or public RND entry points.
 */
#ifndef FB_RT_MATH_RND_STATE_H
#define FB_RT_MATH_RND_STATE_H

/* Include after fb.h. Only the RND providers need this implementation.
   The caller supplies length32 writable words; an empty fill touches nothing.
   Carrying the previous value locally avoids rereading the output vector. */
static __inline__ void hRnd_FillFAST32 ( uint32_t *buffer, uint32_t length32, uint32_t iseed32 )
{
	uint32_t i;

	if( length32 == 0 )
		return;

	buffer[0] = iseed32;
	for( i = 1; i < length32; i++ ) {
		iseed32 = FBRNDFAST32( iseed32 );
		buffer[i] = iseed32;
	}
}

#endif

/* end of math/rnd_state.h */
