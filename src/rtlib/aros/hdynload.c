/*
    FreeBASIC runtime library
    -------------------------

    File: aros/hdynload.c

    Purpose:

        Define the dynamic-library boundary for AROS programs.

    Responsibilities:

        - fail DylibLoad requests without leaking partially resolved symbols
        - preserve the portable runtime's unload contract

    This file intentionally does NOT contain:

        - Unix dlopen() assumptions
        - AROS dynmodule process-protocol glue
        - native library-vector bindings

    Platform constraint:

        AROS dynmodule modules are cooperating executables with generated
        import/export tables and a message-port protocol.  They are not a
        drop-in implementation of dlopen()/dlsym(), and Exec libraries expose
        numbered vectors rather than a general named-symbol table.  Returning
        failure is deterministic and prevents FreeBASIC from presenting either
        mechanism as an ABI-compatible DylibLoad implementation.
*/

#include "../fb.h"
#include "../fb_private_hdynload.h"

FB_DYLIB fb_hDynLoad( const char *libname, const char *const *funcname, void **funcptr )
{
	ssize_t index;

	(void)libname;

	if( funcname != NULL && funcptr != NULL ) {
		for( index = 0; funcname[index] != NULL; index++ )
			funcptr[index] = NULL;
	}

	return NULL;
}

int fb_hDynLoadAlso( FB_DYLIB lib, const char *const *funcname, void **funcptr, ssize_t count )
{
	ssize_t index;

	(void)lib;
	(void)funcname;

	if( funcptr != NULL ) {
		for( index = 0; index < count; index++ )
			funcptr[index] = NULL;
	}

	return -1;
}

void fb_hDynUnload( FB_DYLIB *lib )
{
	if( lib != NULL )
		*lib = NULL;
}

/* end of aros/hdynload.c */
