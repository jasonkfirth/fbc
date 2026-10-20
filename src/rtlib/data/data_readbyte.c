/*
 * Project: FreeBASIC runtime library
 * File: data/data_readbyte.c
 * Purpose: read stmt for byte's.
 * Responsibilities: Compiler-emitted DATA records and typed READ conversion.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL void fb_DataReadByte( char *dst )
{
	FB_LOCK();

	if( __fb_data_ptr ) {
		if( __fb_data_ptr->len == FB_DATATYPE_OFS ) {
			*dst = (size_t)__fb_data_ptr->ofs;
		} else if( __fb_data_ptr->len & FB_DATATYPE_WSTR ) {
			*dst = fb_WstrToInt( __fb_data_ptr->wstr, __fb_data_ptr->len & 0x7FFF );
		} else {
			*dst = fb_hStr2Int( __fb_data_ptr->zstr, __fb_data_ptr->len );
		}
	} else {
		/* no more DATA */
		*dst = 0;
	}

	fb_DataNext( );

	FB_UNLOCK();
}

/* end of data/data_readbyte.c */
