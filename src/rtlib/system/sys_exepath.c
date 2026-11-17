/*
 * Project: FreeBASIC runtime library
 * File: system/sys_exepath.c
 * Purpose: exepath$.
 * Responsibilities: Process, environment, path and operating-system services.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"
#ifdef HOST_WIN32
	#include <windows.h>
#endif

FBCALL FBSTRING_ACCESS *fb_ExePath ( void )
{
	FBSTRING_ACCESS 	*dst;
	char		*p;
	char		tmp[MAX_PATH+1];
	size_t len;

	p = fb_hGetExePath( tmp, MAX_PATH );

	if( p != NULL )
	{
		/* alloc temp string */
        len = strlen( tmp );
		if( len > (size_t)FB_STRSIZEMSK )
			return &fb_ctx.null_desc;
		dst = fb_hStrAllocTemp( NULL, (ssize_t)len );
		if( dst != NULL )
		{
			fb_hStrCopy( dst->data, tmp, (ssize_t)len );
		}
		else
			dst = &fb_ctx.null_desc;
	}
	else
		dst = &fb_ctx.null_desc;

	return dst;
}

/* end of system/sys_exepath.c */
