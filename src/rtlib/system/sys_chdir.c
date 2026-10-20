/*
 * Project: FreeBASIC runtime library
 * File: system/sys_chdir.c
 * Purpose: Provide sys chdir runtime support.
 * Responsibilities: Process, environment, path and operating-system services.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
/*
    FreeBASIC runtime: sys_chdir.c
    Change the working directory and release the BASIC path temporary.
    Native path syntax belongs to the platform's path wrapper.
    This file intentionally does NOT implement filesystem traversal rules.
*/

#include "fb.h"
#ifdef HOST_MINGW
#include <direct.h>
#else
#include <unistd.h>
#endif

FBCALL int fb_ChDir( FBSTRING *path )
{
	int res;

#if defined( HOST_MINGW ) || defined( HOST_AMIGA )
	/* Empty BASIC strings may have no buffer. The wrapper still needs a
	   C string, including when it normalizes native current-directory paths. */
	res = fb_hChangeDir( path->data != NULL ? path->data : "" );
#else
	res = chdir( path->data );
#endif

	/* del if temp */
	fb_hStrDelTemp( path );

	return res;
}

/* end of system/sys_chdir.c */
