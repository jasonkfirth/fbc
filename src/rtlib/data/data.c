/*
 * Project: FreeBASIC runtime library
 * File: data/data.c
 * Purpose: DATA core.
 * Responsibilities: Compiler-emitted DATA records and typed READ conversion.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FB_DATADESC *fb_data_ptr = NULL;

static void hSkipLink( void )
{
	/* If a link was reached, move to the next non-link, or NULL ("EOF") */
	while( fb_data_ptr && (fb_data_ptr->len == FB_DATATYPE_LINK) ) {
		fb_data_ptr = fb_data_ptr->next;
	}

	DBG_ASSERT( (fb_data_ptr == NULL) ||
	            (fb_data_ptr->len != FB_DATATYPE_LINK) );
}

FBCALL void fb_DataRestore( FB_DATADESC *labeladdr )
{
	FB_LOCK();

	fb_data_ptr = labeladdr;
	hSkipLink( );

	FB_UNLOCK();
}

/* Callers are expected to FB_LOCK/FB_UNLOCK */
void fb_DataNext( void )
{
	/* Move forward in current DATA table, if any */
	if( fb_data_ptr ) {
		DBG_ASSERT( fb_data_ptr->len != FB_DATATYPE_LINK );
		fb_data_ptr += 1;
		hSkipLink( );
	}
}

/* end of data/data.c */
