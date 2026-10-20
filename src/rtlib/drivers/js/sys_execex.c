/*
 * Project: FreeBASIC runtime library
 * File: drivers/js/sys_execex.c
 * Purpose: Provide sys execex runtime support.
 * Responsibilities: Target-specific runtime support for js.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

FBCALL int fb_ExecEx( FBSTRING *program, FBSTRING *args, int do_fork )
{
	return fb_ErrorSetNum(FB_RTERROR_FILENOTFOUND);
}

/* end of drivers/js/sys_execex.c */
