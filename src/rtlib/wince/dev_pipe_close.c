/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/dev_pipe_close.c

    Purpose:

        Provide the Windows CE close hook for unsupported pipe files.

    Responsibilities:

        - reject an impossible pipe close operation consistently

    This file intentionally does NOT contain:

        - process synchronization
        - pipe ownership
        - desktop Win32 _pclose() compatibility
*/

#include "../fb.h"

int fb_DevPipeClose( FB_FILE *handle )
{
	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}

/* end of wince/dev_pipe_close.c */
