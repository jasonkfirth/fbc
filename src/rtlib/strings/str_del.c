/*
 * Project: FreeBASIC runtime library
 * File: strings/str_del.c
 * Purpose: string deletion function.
 * Responsibilities: Byte string descriptors, temporaries and string operations.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL void fb_StrDelete ( FBSTRING *str )
{
    if( (str == NULL) || (str->data == NULL) )
    	return;

    free( (void *)str->data );

	str->data = NULL;
	str->len  = 0;
	str->size = 0;
}

/* end of strings/str_del.c */
