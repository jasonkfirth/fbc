/*
    Project: FreeBASIC compiler backend regression tests
    ---------------------------------------------------

    File: integer128-gas32-main.c

    Purpose:

        Provide a process entry point for the 32-bit GAS runtime test.

    Responsibilities:

        - run the GAS constructor before the C entry point
        - turn FreeBASIC runtime failures into a process exit status

    This file intentionally does NOT contain:

        - the 128-bit runtime implementation
        - FreeBASIC startup or console services
*/

#include <stdlib.h>

void fb_End(int exit_code)
{
	exit(exit_code);
}

int main(void)
{
	return 0;
}

/* end of integer128-gas32-main.c */
