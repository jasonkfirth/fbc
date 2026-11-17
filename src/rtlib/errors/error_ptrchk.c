/*
 * Project: FreeBASIC runtime library
 * File: errors/error_ptrchk.c
 * Purpose: null pointer checking function.
 * Responsibilities: Runtime errors, assertions and fault reporting.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"
#include <string.h>

/* The null-check ABI returns BASIC handler addresses through void *. */
typedef char FB_ERROR_HANDLER_ADDRESS_WIDTH[
	(sizeof(FB_ERRHANDLER) == sizeof(void *)) ? 1 : -1];

static void *hErrorHandlerAddress(FB_ERRHANDLER handler)
{
	void *address;

	memcpy((void *)&address, (const void *)&handler, sizeof(address));
	return address;
}

/*:::::*/
FBCALL void *fb_NullPtrChk( void *ptr, int linenum, const char *fname )
{
	if( ptr == NULL )
		return hErrorHandlerAddress(
			fb_ErrorThrowEx(FB_RTERROR_NULLPTR, linenum, fname, NULL, NULL));
	else
		return NULL;
}

/* end of errors/error_ptrchk.c */
