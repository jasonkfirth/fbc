/*
 * Project: FreeBASIC runtime library
 * File: drivers/js/thread_detach.c
 * Purpose: Provide thread detach runtime support.
 * Responsibilities: Target-specific runtime support for js.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

FBCALL void fb_ThreadDetach( FBTHREAD *thread )
{
	if( thread == NULL )
		return;

	free( thread );
}

/* end of drivers/js/thread_detach.c */
