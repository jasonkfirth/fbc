/*
    FreeBASIC Runtime Library
    -------------------------

    File: crt_iob_compat.c

    Purpose:

        Bridge the modern MinGW stdio helper to the C runtime supplied with
        Windows 95-era systems.

    Responsibilities:

        - provide __iob_func() when the target C runtime only has __p__iob()
        - provide __acrt_iob_func() for MinGW's indexed stream access
        - map the modern _stat32 names to their 32-bit Windows 95 equivalents
        - keep the compatibility symbol inside programs that actually need it

    This file intentionally does NOT contain:

        - general C runtime replacements
        - stream allocation or ownership
        - operating-system version detection
*/

/* ------------------------------------------------------------------------- */
/* C runtime compatibility                                                   */
/* ------------------------------------------------------------------------- */

#if defined( __GNUC__ )
	#define FB_CRT_CDECL __attribute__((cdecl))
#else
	#define FB_CRT_CDECL __cdecl
#endif

extern void *FB_CRT_CDECL __p__iob( void );
extern int FB_CRT_CDECL _stat( const char *path, void *status );
extern int FB_CRT_CDECL _wstat( const unsigned short *path, void *status );

typedef int (FB_CRT_CDECL *FB_CRT_STAT_PROC)( const char *path, void *status );
typedef int (FB_CRT_CDECL *FB_CRT_WSTAT_PROC)( const unsigned short *path,
                                               void *status );
typedef void *(FB_CRT_CDECL *FB_CRT_IOB_PROC)( unsigned int index );

enum {
	/* The 32-bit Microsoft FILE layout used by MSVCRT40 is 32 bytes. */
	FB_CRT_FILE_SIZE = 32
};

/*
   Recent MinGW headers route standard streams through __iob_func().  The
   older Microsoft runtime exposes the same stream table through __p__iob().
   Returning that table preserves the expected FILE pointer ABI without
   requiring a newer MSVCRT DLL on Windows 95.
*/
void *FB_CRT_CDECL __iob_func( void )
{
	return __p__iob( );
}

/*
   Current MinGW headers obtain stdin, stdout, and stderr by indexing the
   stream table through __acrt_iob_func().  MSVCRT40 predates that helper, but
   its FILE entries use the same 32-byte layout expected by 32-bit MinGW.
*/
void *FB_CRT_CDECL __acrt_iob_func( unsigned int index )
{
	unsigned char *streams = (unsigned char *)__iob_func( );

	return streams + (index * FB_CRT_FILE_SIZE);
}

/*
   The numbered names only distinguish the time_t layout in newer Microsoft
   runtimes.  Windows 95 and its MSVCRT40 use 32-bit time_t, so their _stat
   and _wstat structures already have the layout requested by _stat32.
*/
int FB_CRT_CDECL _stat32( const char *path, void *status )
{
	return _stat( path, status );
}

int FB_CRT_CDECL _wstat32( const unsigned short *path, void *status )
{
	return _wstat( path, status );
}

/*
   MinGW declares the numbered functions with dllimport and therefore emits
   references to their import-pointer symbols.  Bind those pointers to the
   local adapters so existing runtime objects do not need to be rebuilt with
   alternate system headers.
*/
FB_CRT_STAT_PROC fb_CrtImpStat32 __asm__("__imp___stat32") = _stat32;
FB_CRT_WSTAT_PROC fb_CrtImpWstat32 __asm__("__imp___wstat32") = _wstat32;
FB_CRT_IOB_PROC fb_CrtImpAcrtIobFunc
	__asm__("__imp____acrt_iob_func") = __acrt_iob_func;

#undef FB_CRT_CDECL

/* end of crt_iob_compat.c */
