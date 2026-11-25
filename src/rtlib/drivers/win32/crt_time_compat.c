/*
    Project: FreeBASIC Runtime Library
    -------------------------

    File: drivers/win32/crt_time_compat.c

    Purpose:

        Bridge MinGW's 32-bit time imports to the time functions exported by
        the Windows 95-era C runtime.

    Responsibilities:

        - provide the 32-bit time and local-time symbols used by MinGW
        - preserve the 32-bit time_t layout required by MSVCRT40
        - keep compatibility symbols inside programs that need them

    This file intentionally does NOT contain:

        - timezone conversion policy
        - date arithmetic or FreeBASIC time statements
        - 64-bit time support
*/

/* ------------------------------------------------------------------------- */
/* Legacy CRT time aliases                                                   */
/* ------------------------------------------------------------------------- */

#if defined( __i386__ ) || defined( _M_IX86 )

#if defined( __GNUC__ )
	#define FB_CRT_CDECL __attribute__((cdecl))
	#define FB_CRT_WEAK __attribute__((weak))
#else
	#define FB_CRT_CDECL __cdecl
	#define FB_CRT_WEAK
#endif

typedef long FB_CRT_TIME32;
struct tm;

/*
   A 32-bit MinGW target uses 32-bit time_t.  MSVCRT40 exports the unnumbered
   functions with that same layout, so these calls preserve the CRT ABI while
   satisfying MinGW's newer numbered symbol names.
*/
extern FB_CRT_TIME32 FB_CRT_CDECL fb_CrtLegacyTime( FB_CRT_TIME32 *timer )
	__asm__( "_time" );
extern struct tm *FB_CRT_CDECL fb_CrtLegacyLocaltime(
	const FB_CRT_TIME32 *timer ) __asm__( "_localtime" );

FB_CRT_TIME32 FB_CRT_CDECL fb_CrtTime32( FB_CRT_TIME32 *timer )
	__asm__( "__time32" ) FB_CRT_WEAK;
FB_CRT_TIME32 FB_CRT_CDECL fb_CrtTime32( FB_CRT_TIME32 *timer )
{
	return fb_CrtLegacyTime( timer );
}

struct tm *FB_CRT_CDECL fb_CrtLocaltime32( const FB_CRT_TIME32 *timer )
	__asm__( "__localtime32" ) FB_CRT_WEAK;
struct tm *FB_CRT_CDECL fb_CrtLocaltime32( const FB_CRT_TIME32 *timer )
{
	return fb_CrtLegacyLocaltime( timer );
}

#undef FB_CRT_CDECL
#undef FB_CRT_WEAK

#else
/* Keep the Win64 translation unit valid without defining 32-bit symbols. */
typedef int fb_crt_time_compat_empty_translation_unit;
#endif

/* end of drivers/win32/crt_time_compat.c */
