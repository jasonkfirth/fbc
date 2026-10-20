/*
 * Project: FreeBASIC runtime library
 * File: drivers/js/file_dir.c
 * Purpose: dir().
 * Responsibilities: Target-specific runtime support for js.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

FBCALL FBSTRING *fb_Dir( FBSTRING *filespec, int attrib, int *out_attrib )
{
	return &__fb_ctx.null_desc;
}

/* end of drivers/js/file_dir.c */
