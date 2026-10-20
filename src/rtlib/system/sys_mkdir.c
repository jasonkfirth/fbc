/*
 * Project: FreeBASIC runtime library
 * File: system/sys_mkdir.c
 * Purpose: mkdir function.
 * Responsibilities: Process, environment, path and operating-system services.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"
#ifdef HOST_MINGW
#include <direct.h>
#else
#include <sys/stat.h>
#endif

/*:::::*/
FBCALL int fb_MkDir( FBSTRING *path )
{
	int res;

#ifdef HOST_MINGW
	res = fb_hMakeDir( path->data );
#else
	res = mkdir( path->data, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH );
#endif

	/* del if temp */
	fb_hStrDelTemp( path );

	return res;
}

/* end of system/sys_mkdir.c */
