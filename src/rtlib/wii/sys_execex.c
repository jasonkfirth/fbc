/*
    FreeBASIC runtime Wii process execution stub
    --------------------------------------------

    File: sys_execex.c

    Purpose:

        Provide a defined result for EXEC, RUN, and CHAIN on Wii.

    Responsibilities:

        - release temporary string descriptors passed by the compiler
        - report that starting another executable is unsupported
        - return -1 so RUN does not terminate the current program

    This file intentionally does NOT contain:

        - DOL loading
        - application relaunch support
        - process creation
*/

#include "../fb.h"

FBCALL int fb_ExecEx(FBSTRING *program, FBSTRING *args, int do_fork)
{
	(void)do_fork;

	fb_hStrDelTemp(args);
	fb_hStrDelTemp(program);
	fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);

	return -1;
}

/* end of sys_execex.c */
