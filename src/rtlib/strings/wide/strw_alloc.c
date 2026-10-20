/*
 * Project: FreeBASIC runtime library
 * File: strings/wide/strw_alloc.c
 * Purpose: wstring allocation function.
 * Responsibilities: Wide string operations using the target FB_WCHAR width.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL FB_WCHAR *fb_WstrAlloc( ssize_t chars )
{
	if( chars <= 0 )
		return NULL;

	return fb_wstr_AllocTemp( chars );
}

/* end of strings/wide/strw_alloc.c */
