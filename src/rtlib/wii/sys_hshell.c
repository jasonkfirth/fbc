/*
    FreeBASIC runtime Wii shell stub
    --------------------------------

    File: sys_hshell.c

    Purpose:

        Provide a defined failure for the SHELL statement on Wii.

    Responsibilities:

        - export the shell hook required by the generic runtime
        - report FB_RTERROR_ILLEGALFUNCTIONCALL

    This file intentionally does NOT contain:

        - command interpreter emulation
        - DOL loading
        - process creation
*/

#include "../fb.h"

int fb_hShell(char *program)
{
	(void)program;

	fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	return -1;
}

/* end of sys_hshell.c */
