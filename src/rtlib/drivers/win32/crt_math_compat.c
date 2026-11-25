/*
    Project: FreeBASIC Runtime Library
    -------------------------

    File: drivers/win32/crt_math_compat.c

    Purpose:

        Provide MinGW's combined sine/cosine helper on the Windows 95-era C
        runtime, which exports the individual legacy math functions.

    Responsibilities:

        - satisfy the 32-bit sincosf symbol emitted by MinGW
        - calculate results through the CRT's sin and cos functions
        - keep the fallback inside programs that need the helper

    This file intentionally does NOT contain:

        - general math-library replacements
        - trigonometric range reduction
        - operating-system version detection
*/

/* ------------------------------------------------------------------------- */
/* Legacy CRT math aliases                                                   */
/* ------------------------------------------------------------------------- */

#if defined( __i386__ ) || defined( _M_IX86 )

#if defined( __GNUC__ )
	#define FB_CRT_CDECL __attribute__((cdecl))
	#define FB_CRT_WEAK __attribute__((weak))
#else
	#define FB_CRT_CDECL __cdecl
	#define FB_CRT_WEAK
#endif

extern double FB_CRT_CDECL fb_CrtLegacySin( double value )
	__asm__( "_sin" );
extern double FB_CRT_CDECL fb_CrtLegacyCos( double value )
	__asm__( "_cos" );

void FB_CRT_CDECL fb_CrtSincosf( float value, float *sine, float *cosine )
	__asm__( "_sincosf" ) FB_CRT_WEAK;
void FB_CRT_CDECL fb_CrtSincosf( float value, float *sine, float *cosine )
{
	if( sine != 0 )
		*sine = (float)fb_CrtLegacySin( (double)value );
	if( cosine != 0 )
		*cosine = (float)fb_CrtLegacyCos( (double)value );
}

#undef FB_CRT_CDECL
#undef FB_CRT_WEAK

#else
/* Keep the Win64 translation unit valid without defining 32-bit symbols. */
typedef int fb_crt_math_compat_empty_translation_unit;
#endif

/* end of drivers/win32/crt_math_compat.c */
