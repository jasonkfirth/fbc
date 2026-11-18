/*
 * Project: FreeBASIC runtime library
 * File: data/data_readstr.c
 * Purpose: read stmt for strings.
 * Responsibilities: Compiler-emitted DATA records and typed READ conversion.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL void fb_DataReadStr( void *dst, ssize_t dst_size, int fillrem )
{
	char empty_string[] = "";

	FB_LOCK();

	if( fb_data_ptr ) {
		if( fb_data_ptr->len == FB_DATATYPE_OFS ) {
			FBSTRING_ACCESS *text = fb_ULongintToStr( (unsigned long long)(uintptr_t)fb_data_ptr->ofs );
			fb_StrAssign( dst, dst_size, text, -1, fillrem );
		} else if( fb_data_ptr->len & FB_DATATYPE_WSTR ) {
			fb_WstrAssignToA( dst, dst_size, fb_data_ptr->wstr, fillrem );
		} else {
			fb_StrAssign( dst, dst_size, fb_data_ptr->zstr, 0, fillrem );
		}
	} else {
		/* no more DATA, return empty string */
		fb_StrAssign( dst, dst_size, empty_string, 0, fillrem );
	}

	fb_DataNext( );

	FB_UNLOCK();
}

/* end of data/data_readstr.c */
