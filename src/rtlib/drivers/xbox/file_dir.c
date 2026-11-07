/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/file_dir.c
 * Purpose: dir().
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

typedef struct _FB_DIRCTX {
	int in_use;
} FB_DIRCTX;

void fb_DIRCTX_Destructor( void *data )
{
	(void)data;
}

FBCALL FBSTRING_ACCESS *fb_Dir( FBSTRING_ACCESS *filespec, int attrib, int *out_attrib )
{
	(void)attrib;

	if( out_attrib != NULL )
		*out_attrib = 0;

	fb_hStrDelTemp( filespec );

	return &fb_ctx.null_desc;
}

/* end of drivers/xbox/file_dir.c */
