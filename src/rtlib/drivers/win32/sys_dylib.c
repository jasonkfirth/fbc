/*
 * Project: FreeBASIC runtime library
 * File: drivers/win32/sys_dylib.c
 * Purpose: Dynamic library loading functions.
 * Responsibilities: Target-specific runtime support for win32.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <windows.h>
#include <string.h>

/*
    The exported dynamic-symbol API stores Win32 procedure addresses as
    void*. Preserve that representation without converting between pointer
    kinds in C.
*/
typedef char FB_WIN32_DYLIB_POINTER_SIZE_CHECK[
	(sizeof(FARPROC) == sizeof(void *)) ? 1 : -1];

static void *fb_hDylibGetSymbol(HMODULE library, const char *name)
{
	FARPROC procedure = GetProcAddress(library, name);
	void *address = NULL;

	if (procedure != NULL)
		memcpy((void *)&address, (const void *)&procedure, sizeof(address));

	return address;
}

FBCALL void *fb_DylibLoad( FBSTRING_ACCESS *library )
{
	void *res = NULL;

	if( (library) && (library->data) )
		res = LoadLibrary( library->data );

	/* del if temp */
	fb_hStrDelTemp( library );

	return res;
}

FBCALL void *fb_DylibSymbol( void *library, FBSTRING_ACCESS *symbol )
{
	void *proc = NULL;
	char procname[1024];
	int i;

	if( library == NULL )
		library = GetModuleHandle( NULL );

	if( (symbol) && (symbol->data) )
	{
		proc = fb_hDylibGetSymbol((HINSTANCE)library, symbol->data);
		if( (!proc) && (!strchr( symbol->data, '@' )) ) {
			procname[1023] = '\0';
			for( i = 0; i < 256; i += 4 ) {
				snprintf( procname, 1023, "%s@%d", symbol->data, i );
				proc = fb_hDylibGetSymbol((HINSTANCE)library, procname);
				if( proc )
					break;
			}
		}
	}

	/* del if temp */
	fb_hStrDelTemp( symbol );

	return proc;
}

FBCALL void *fb_DylibSymbolByOrd ( void *library, short int symbol )
{
	void *proc = NULL;

	if( library == NULL )
		library = GetModuleHandle( NULL );

	proc = fb_hDylibGetSymbol((HINSTANCE)library, MAKEINTRESOURCE(symbol));

	return proc;
}

FBCALL void fb_DylibFree( void *library )
{
	FreeLibrary((HINSTANCE) library);
}

/* end of drivers/win32/sys_dylib.c */
