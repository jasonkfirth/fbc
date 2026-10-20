/*
 * Project: FreeBASIC runtime library
 * File: drivers/win32/sys_hsecure_random.c
 * Purpose: Provide sys hsecure random runtime support.
 * Responsibilities: Target-specific runtime support for win32.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
/*
 * FreeBASIC Win32 runtime: sys_hsecure_random.c
 * Obtain checked bytes from Windows' system-preferred CNG provider.
 * Each call owns its loader reference and uses no shared mutable state.
 * This file contains no PRNG, fallback seed or dependency on RND globals.
 */

#include "../../fb.h"
#include "../../fb_secure_random.h"
#include <windows.h>
#include <bcrypt.h>

#ifndef LOAD_LIBRARY_SEARCH_SYSTEM32
/* Windows SDK loader flag; unavailable older systems fail the load. */
#define LOAD_LIBRARY_SEARCH_SYSTEM32 0x00000800
#endif

int fb_hSecureRandom( void *destination, size_t byte_count )
{
	typedef NTSTATUS (WINAPI *GENERATE)( BCRYPT_ALG_HANDLE, PUCHAR, ULONG, ULONG );
	const WCHAR suffix[] = L"\\bcrypt.dll";
	WCHAR path[MAX_PATH];
	UINT length;
	HMODULE module;
	FARPROC address;
	GENERATE generate;
	int status;

	length = GetSystemDirectoryW( path, MAX_PATH );
	if( length == 0 || length >= MAX_PATH ||
	    sizeof( suffix ) / sizeof( suffix[0] ) > MAX_PATH - length )
		return FB_SECURE_RANDOM_IO_ERROR;
	memcpy( path + length, suffix, sizeof( suffix ) );

	/* Use the system directory and restricted dependency lookup. A missing
	 * provider remains an explicit failure instead of searching the application
	 * directory or installing a new mandatory runtime DLL.
	 */
	module = LoadLibraryExW( path, NULL, LOAD_LIBRARY_SEARCH_SYSTEM32 );
	if( module == NULL )
		return FB_SECURE_RANDOM_UNAVAILABLE;
	address = GetProcAddress( module, "BCryptGenRandom" );
	status = FB_SECURE_RANDOM_UNAVAILABLE;
	if( address != NULL ) {
		/* The Windows loader returns a callable function pointer. Copy its
		 * representation without an incompatible function-pointer cast.
		 */
		STATIC_ASSERT( sizeof( generate ) == sizeof( address ) );
		memcpy( &generate, &address, sizeof( generate ) );
		status = generate( NULL, destination, (ULONG)byte_count,
		                   BCRYPT_USE_SYSTEM_PREFERRED_RNG ) == 0 ?
		         FB_SECURE_RANDOM_OK : FB_SECURE_RANDOM_IO_ERROR;
	}
	if( FreeLibrary( module ) == 0 )
		status = FB_SECURE_RANDOM_IO_ERROR;
	return status;
}

/* end of drivers/win32/sys_hsecure_random.c */
