/*
 * Project: FreeBASIC runtime library
 * File: system/sys_run.c
 * Purpose: RUN function.
 * Responsibilities: Process, environment, path and operating-system services.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL int fb_Run( FBSTRING *program, FBSTRING *args )
{
	if( fb_ExecEx( program, args, FALSE ) != -1 )
		fb_End( 0 );

    return -1;
}

/* end of system/sys_run.c */
