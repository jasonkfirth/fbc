/*
 * Project: FreeBASIC runtime library
 * File: data/data_readwstr.c
 * Purpose: read stmt for wstring's.
 * Responsibilities: Compiler-emitted DATA records and typed READ conversion.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL void fb_DataReadWstr( FB_WCHAR *dst, ssize_t dst_size )
{
	FB_WCHAR empty_string[] = FB_WCHAR_LITERAL("");

	FB_LOCK();

	if( fb_data_ptr ) {
		if( fb_data_ptr->len == FB_DATATYPE_OFS ) {
			/* !!!WRITEME!!! */
		} else if( fb_data_ptr->len & FB_DATATYPE_WSTR ) {
			fb_WstrAssign( dst, dst_size, fb_data_ptr->wstr );
		} else {
			fb_WstrAssignFromA( dst, dst_size, fb_data_ptr->zstr, fb_data_ptr->len );
		}
	} else {
		/* no more DATA, return empty string */
		fb_WstrAssign( dst, dst_size, empty_string );
	}

	fb_DataNext( );

	FB_UNLOCK();
}

/* end of data/data_readwstr.c */
