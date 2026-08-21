/*
    FreeBASIC runtime library
    -------------------------

    File: aros/sys_dylib.c

    Purpose:

        Provide deterministic DylibLoad behavior on AROS.

    Responsibilities:

        - release temporary BASIC strings
        - report that POSIX-style named-symbol loading is unavailable

    This file intentionally does NOT contain:

        - Unix dlopen() filename probing
        - AROS Exec library vector bindings
        - dynmodule import/export table generation
*/

#include "../fb.h"

FBCALL void *fb_DylibLoad( FBSTRING *library )
{
	fb_hStrDelTemp( library );
	return NULL;
}

FBCALL void *fb_DylibSymbol( void *library, FBSTRING *symbol )
{
	(void)library;
	fb_hStrDelTemp( symbol );
	return NULL;
}

FBCALL void *fb_DylibSymbolByOrd( void *library, short int symbol )
{
	(void)library;
	(void)symbol;
	return NULL;
}

FBCALL void fb_DylibFree( void *library )
{
	(void)library;
}

/* end of aros/sys_dylib.c */
