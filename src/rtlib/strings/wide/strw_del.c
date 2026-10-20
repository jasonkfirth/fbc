/*
 * Project: FreeBASIC runtime library
 * File: strings/wide/strw_del.c
 * Purpose: wstring deletion function.
 * Responsibilities: Wide string operations using the target FB_WCHAR width.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL void fb_WstrDelete ( FB_WCHAR *str )
{
    if( str == NULL )
    	return;

    fb_wstr_Del( str );
}

/* end of strings/wide/strw_del.c */
