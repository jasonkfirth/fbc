/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/sys_dylib.c

    Purpose:

        Implement DYLIB operations through Windows CE's Unicode loader API.

    Responsibilities:

        - convert library and symbol names to UTF-16
        - search undecorated and stdcall-decorated symbol spellings
        - support symbol lookup by ordinal
        - release temporary FreeBASIC strings

    This file intentionally does NOT contain:

        - desktop ANSI loader calls
        - library search-path policy
        - automatic dependency installation
*/

#include "../fb.h"

#include <windows.h>

static FARPROC hGetSymbol( HINSTANCE library, const char *name )
{
	wchar_t *wide_name;
	FARPROC procedure;

	wide_name = fb_hWinCEToWC( name );
	if( wide_name == NULL )
		return NULL;

	procedure = GetProcAddressW( library, wide_name );
	free( wide_name );

	return procedure;
}

FBCALL void *fb_DylibLoad( FBSTRING *library )
{
	wchar_t *wide_name = NULL;
	HINSTANCE result = NULL;

	if( (library != NULL) && (library->data != NULL) ) {
		wide_name = fb_hConvertPathToWC( library->data, NULL );
		if( wide_name != NULL )
			result = LoadLibraryW( wide_name );
	}

	free( wide_name );
	fb_hStrDelTemp( library );

	return result;
}

FBCALL void *fb_DylibSymbol( void *library, FBSTRING *symbol )
{
	HINSTANCE module = (HINSTANCE)library;
	FARPROC procedure = NULL;
	char decorated_name[1024];
	int argument_bytes;
	int decorated_length;

	if( module == NULL )
		module = GetModuleHandleW( NULL );

	if( (symbol != NULL) && (symbol->data != NULL) ) {
		procedure = hGetSymbol( module, symbol->data );
		if( (procedure == NULL) && (strchr( symbol->data, '@' ) == NULL) ) {
			for( argument_bytes = 0; argument_bytes < 256;
			     argument_bytes += 4 ) {
				decorated_length = snprintf( decorated_name,
				                             sizeof( decorated_name ), "%s@%d",
				                             symbol->data, argument_bytes );
				if( (decorated_length < 0) ||
				    ((size_t)decorated_length >= sizeof( decorated_name )) ) {
					break;
				}

				procedure = hGetSymbol( module, decorated_name );
				if( procedure != NULL )
					break;
			}
		}
	}

	fb_hStrDelTemp( symbol );
	return (void *)procedure;
}

FBCALL void *fb_DylibSymbolByOrd( void *library, short int symbol )
{
	HINSTANCE module = (HINSTANCE)library;

	if( module == NULL )
		module = GetModuleHandleW( NULL );

	return (void *)GetProcAddressW( module,
	                                MAKEINTRESOURCEW( (WORD)symbol ) );
}

FBCALL void fb_DylibFree( void *library )
{
	if( library != NULL )
		FreeLibrary( (HINSTANCE)library );
}

/* end of wince/sys_dylib.c */
