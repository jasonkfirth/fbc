/*
    Project: FreeBASIC Runtime Library
    -------------------------

    File: drivers/win32/crt_iob_func_compat.c

    Purpose:

        Provide the MinGW __iob_func() entry point when targeting an older
        32-bit Windows C runtime that only exposes __p__iob().

    Responsibilities:

        - adapt the legacy stream table to the MinGW stdio entry point
        - keep the fallback in its own archive member so a newer CRT's
          implementation is selected when it is available

    This file intentionally does NOT contain:

        - indexed __acrt_iob_func() compatibility
        - numbered stat compatibility
        - stream allocation or ownership
*/

/* ------------------------------------------------------------------------- */
/* Legacy standard-stream entry point                                        */
/* ------------------------------------------------------------------------- */

#if defined( __i386__ ) || defined( _M_IX86 )

#if defined( __GNUC__ )
	#define FB_CRT_CDECL __attribute__((cdecl))
#else
	#define FB_CRT_CDECL __cdecl
#endif

extern void *FB_CRT_CDECL __p__iob( void );

/*
   Keep this fallback in a separate object file. Static linkers extract an
   archive member only when __iob_func() is unresolved, so newer CRTs can
   provide their implementation without colliding with this legacy bridge.
*/
void *FB_CRT_CDECL __iob_func( void )
{
	return __p__iob( );
}

#undef FB_CRT_CDECL

#endif

/* end of drivers/win32/crt_iob_func_compat.c */
