/*
    FreeBASIC runtime Wii pipe stubs
    --------------------------------

    File: pipe_stubs.c

    Purpose:

        Provide popen()/pclose() symbols for the shared pipe runtime on
        Wii, where there is no host shell process model.

    Responsibilities:

        - fail pipe opens explicitly with ENOSYS

    This file intentionally does NOT contain:

        - command shell emulation
        - IPC pipe implementation
*/

#include <errno.h>
#include <stdio.h>

FILE *popen(const char *command, const char *mode)
{
	errno = ENOSYS;
	return NULL;
}

int pclose(FILE *stream)
{
	errno = ENOSYS;
	return -1;
}

/* end of pipe_stubs.c */
