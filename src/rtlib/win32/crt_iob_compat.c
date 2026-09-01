/*
    FreeBASIC Runtime Library
    -------------------------

    File: crt_iob_compat.c

    Purpose:

        Bridge the modern MinGW stdio helper to the C runtime supplied with
        Windows 95-era systems.

    Responsibilities:

        - provide __iob_func() when 32-bit x86 uses an older target C runtime
        - route MinGW's indexed stream access to the Windows 95 stream table
        - map numbered stat imports to their 32-bit Windows 95 equivalents
        - keep the compatibility symbol inside programs that actually need it

    This file intentionally does NOT contain:

        - general C runtime replacements
        - stream allocation or ownership
        - operating-system version detection
*/

/* ------------------------------------------------------------------------- */
/* C runtime compatibility                                                   */
/* ------------------------------------------------------------------------- */

#if defined( __i386__ ) || defined( _M_IX86 )

#if defined( __GNUC__ )
	#define FB_CRT_CDECL __attribute__((cdecl))
	#define FB_CRT_WEAK __attribute__((weak))
#else
	#define FB_CRT_CDECL __cdecl
	#define FB_CRT_WEAK
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
void *FB_CRT_CDECL FB_CRT_WEAK __iob_func( void )
{
	return __p__iob( );
}

/*
   Current MinGW headers obtain stdin, stdout, and stderr by indexing the
   stream table through __acrt_iob_func().  MSVCRT40 predates that helper, but
   its FILE entries use the same 32-byte layout expected by 32-bit MinGW.
*/
static void *FB_CRT_CDECL fb_CrtAcrtIobFunc( unsigned int index )
{
	unsigned char *streams = (unsigned char *)__iob_func( );

	return streams + (index * FB_CRT_FILE_SIZE);
}

/*
   The numbered names only distinguish the time_t layout in newer Microsoft
   runtimes.  Windows 95 and its MSVCRT40 use 32-bit time_t, so their _stat
   and _wstat structures already have the layout requested by _stat32.
*/
static int FB_CRT_CDECL fb_CrtStat32( const char *path, void *status )
{
	return _stat( path, status );
}

static int FB_CRT_CDECL fb_CrtWstat32( const unsigned short *path, void *status )
{
	return _wstat( path, status );
}

/*
   MinGW declares the numbered functions with dllimport and therefore emits
   references to their import-pointer symbols.  Weak definitions let a modern
   MinGW CRT replace these pointers while retaining the local fallback when a
   Windows 95 package links only against MSVCRT40.
*/
FB_CRT_STAT_PROC fb_CrtImpStat32
	__asm__("__imp___stat32") FB_CRT_WEAK = fb_CrtStat32;
FB_CRT_WSTAT_PROC fb_CrtImpWstat32
	__asm__("__imp___wstat32") FB_CRT_WEAK = fb_CrtWstat32;
FB_CRT_IOB_PROC fb_CrtImpAcrtIobFunc
	__asm__("__imp____acrt_iob_func") FB_CRT_WEAK = fb_CrtAcrtIobFunc;

#undef FB_CRT_CDECL
#undef FB_CRT_WEAK

#endif

/* end of crt_iob_compat.c */
