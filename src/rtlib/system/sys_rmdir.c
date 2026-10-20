/*
 * Project: FreeBASIC runtime library
 * File: system/sys_rmdir.c
 * Purpose: rmdir function.
 * Responsibilities: Process, environment, path and operating-system services.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"
#ifdef HOST_MINGW
#include <direct.h>
#else
#include <unistd.h>
#endif

/*:::::*/
FBCALL int fb_RmDir( FBSTRING *path )
{
	int res;

#ifdef HOST_MINGW
	res = fb_hRemoveDir( path->data );
#else
	res = rmdir( path->data );
#endif

	/* del if temp */
	fb_hStrDelTemp( path );

	return res;
}

/* end of system/sys_rmdir.c */
