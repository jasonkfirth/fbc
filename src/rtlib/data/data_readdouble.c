/*
 * Project: FreeBASIC runtime library
 * File: data/data_readdouble.c
 * Purpose: read stmt for double's.
 * Responsibilities: Compiler-emitted DATA records and typed READ conversion.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL void fb_DataReadDouble( double *dst )
{
	FB_LOCK();

	if( __fb_data_ptr ) {
		if( __fb_data_ptr->len == FB_DATATYPE_OFS ) {
			*dst = (size_t)__fb_data_ptr->ofs;
		} else if( __fb_data_ptr->len & FB_DATATYPE_WSTR ) {
			*dst = fb_WstrToDouble( __fb_data_ptr->wstr, __fb_data_ptr->len & 0x7FFF );
		} else {
			*dst = fb_hStr2Double( __fb_data_ptr->zstr, __fb_data_ptr->len );
		}
	} else {
		/* no more DATA */
		*dst = 0.0;
	}

	fb_DataNext( );

	FB_UNLOCK();
}

/* end of data/data_readdouble.c */
