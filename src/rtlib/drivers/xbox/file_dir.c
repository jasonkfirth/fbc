/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/file_dir.c
 * Purpose: dir().
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "file_path.h"

typedef struct _FB_DIRCTX {
	int in_use;
	unsigned int attrib;
	HANDLE handle;
	WIN32_FIND_DATAA data;
	char mask[MAX_PATH];
} FB_DIRCTX;

static void close_dir( FB_DIRCTX *ctx )
{
	if( ctx->in_use )
		FindClose( ctx->handle );
	ctx->in_use = FALSE;
	ctx->handle = INVALID_HANDLE_VALUE;
}

void fb_DIRCTX_Destructor( void *data )
{
	close_dir( (FB_DIRCTX *)data );
}

FBCALL FBSTRING_ACCESS *fb_Dir( FBSTRING_ACCESS *filespec, int attrib, int *out_attrib )
{
	FB_DIRCTX *ctx;
	FBSTRING_ACCESS *result = &fb_ctx.null_desc;
	int found = FALSE;
	size_t length;

	if( out_attrib != NULL )
		*out_attrib = 0;

	if( filespec == NULL )
		return result;
	ctx = FB_TLSGETCTX( DIR );
	if( ctx == NULL ) {
		fb_hStrDelTemp( filespec );
		return result;
	}
	if( FB_STRSIZE(filespec) > 0 ) {
		close_dir( ctx );
		ctx->handle = fb_hFindFirstFile( filespec->data, &ctx->data, ctx->mask, sizeof(ctx->mask) );
		ctx->in_use = ctx->handle != INVALID_HANDLE_VALUE;
		/* As on Win32, ignore native high attribute bits and allow the
		   archive bit for ordinary files. Enumeration state is per thread. */
		/* Native NORMAL is a marker for the absence of special attributes,
		   rather than an extra BASIC attribute which callers must request. */
		ctx->attrib = (unsigned int)attrib | 0xFFFFFF00u | FILE_ATTRIBUTE_NORMAL;
		if( (attrib & 0x10) == 0 ) ctx->attrib |= 0x20;
		found = ctx->in_use;
	} else if( ctx->in_use ) {
		found = fb_hFindNextFile( ctx->handle, ctx->mask, &ctx->data );
	}
	while( found && (ctx->data.dwFileAttributes & ~ctx->attrib) )
		found = fb_hFindNextFile( ctx->handle, ctx->mask, &ctx->data );
	if( !found )
		close_dir( ctx );

	FB_STRLOCK();
	if( found ) {
		ctx->data.cFileName[sizeof(ctx->data.cFileName)-1] = '\0';
		length = strlen( ctx->data.cFileName );
		result = fb_hStrAllocTemp_NoLock( NULL, length );
		if( result != NULL ) {
			fb_hStrCopy( result->data, ctx->data.cFileName, length );
			if( out_attrib != NULL ) *out_attrib = ctx->data.dwFileAttributes & 0x7F;
		} else {
			close_dir( ctx );
			result = &fb_ctx.null_desc;
		}
	}
	fb_hStrDelTemp_NoLock( filespec );
	FB_STRUNLOCK();

	return result;
}

/* end of drivers/xbox/file_dir.c */
