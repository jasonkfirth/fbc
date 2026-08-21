/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/hdynload.c

    Purpose:

        Load optional runtime dependencies through Windows CE's loader.

    Responsibilities:

        - convert library paths and exported names to UTF-16
        - resolve complete required symbol tables atomically
        - unload partially initialized libraries on failure

    This file intentionally does NOT contain:

        - public DYLIB statement handling
        - desktop ANSI loader calls
        - fallback library-name probing
*/

#include "../fb.h"
#include "../fb_private_hdynload.h"

#include <windows.h>

static FB_DYLIB hLoadLibrary( const char *name )
{
	wchar_t *wide_name;
	FB_DYLIB library;

	wide_name = fb_hConvertPathToWC( name, NULL );
	if( wide_name == NULL )
		return NULL;

	library = LoadLibraryW( wide_name );
	free( wide_name );

	return library;
}

static void *hLoadSymbol( FB_DYLIB library, const char *name )
{
	wchar_t *wide_name;
	FARPROC procedure;

	wide_name = fb_hWinCEToWC( name );
	if( wide_name == NULL )
		return NULL;

	procedure = GetProcAddressW( library, wide_name );
	free( wide_name );

	return (void *)procedure;
}

FB_DYLIB fb_hDynLoad( const char *library_name,
	                  const char *const *function_names,
	                  void **function_pointers )
{
	FB_DYLIB library;
	ssize_t index;

	library = hLoadLibrary( library_name );
	if( library == NULL )
		return NULL;

	for( index = 0; function_names[index] != NULL; ++index ) {
		function_pointers[index] = hLoadSymbol( library,
		                                       function_names[index] );
		if( function_pointers[index] == NULL ) {
			FreeLibrary( library );
			return NULL;
		}
	}

	return library;
}

int fb_hDynLoadAlso( FB_DYLIB library,
	                 const char *const *function_names,
	                 void **function_pointers, ssize_t count )
{
	ssize_t index;

	for( index = 0; index < count; ++index ) {
		function_pointers[index] = hLoadSymbol( library,
		                                       function_names[index] );
		if( function_pointers[index] == NULL )
			return -1;
	}

	return 0;
}

void fb_hDynUnload( FB_DYLIB *library )
{
	if( (library != NULL) && (*library != NULL) ) {
		FreeLibrary( *library );
		*library = NULL;
	}
}

/* end of wince/hdynload.c */
