/*
 * Project: FreeBASIC runtime library
 * File: data/data_readulong.c
 * Purpose: read stmt for ulong integer's.
 * Responsibilities: Compiler-emitted DATA records and typed READ conversion.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL void fb_DataReadULongint( unsigned long long *dst )
{
	FB_LOCK();

	if( fb_data_ptr ) {
		if( fb_data_ptr->len == FB_DATATYPE_OFS ) {
			*dst = (size_t)fb_data_ptr->ofs;
		} else if( fb_data_ptr->len & FB_DATATYPE_WSTR ) {
			*dst = fb_WstrToULongint( fb_data_ptr->wstr, fb_data_ptr->len & 0x7FFF );
		} else {
			*dst = fb_hStr2ULongint( fb_data_ptr->zstr, fb_data_ptr->len );
		}
	} else {
		/* no more DATA */
		*dst = 0;
	}

	fb_DataNext( );

	FB_UNLOCK();
}

/* end of data/data_readulong.c */
