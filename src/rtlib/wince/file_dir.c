/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/file_dir.c

    Purpose:

        Implement DIR with the native Windows CE Unicode search API.

    Responsibilities:

        - maintain one directory search per runtime thread
        - convert FreeBASIC path strings to and from UTF-16
        - filter results using FreeBASIC's requested attribute mask
        - close search handles at exhaustion and thread teardown

    This file intentionally does NOT contain:

        - desktop CRT directory calls
        - ANSI Windows API fallbacks
        - recursive directory traversal
*/

#include "../fb.h"

#include <windows.h>

typedef struct FB_DIRCTX {
	int in_use;
	DWORD allowed_attributes;
	int return_utf8;
	WIN32_FIND_DATAW data;
	HANDLE handle;
} FB_DIRCTX;

static void hCloseSearch( FB_DIRCTX *context )
{
	if( context->in_use )
		FindClose( context->handle );

	context->handle = INVALID_HANDLE_VALUE;
	context->in_use = FALSE;
}

void fb_DIRCTX_Destructor( void *data )
{
	FB_DIRCTX *context = (FB_DIRCTX *)data;

	hCloseSearch( context );
}

static int hAttributesAllowed( const FB_DIRCTX *context )
{
	return (context->data.dwFileAttributes &
	        ~context->allowed_attributes) == 0;
}

static char *hFindNext( FB_DIRCTX *context, int *attributes )
{
	char *name;

	while( FindNextFileW( context->handle, &context->data ) ) {
		if( !hAttributesAllowed( context ) )
			continue;

		name = fb_hConvertPathFromWC( context->data.cFileName,
		                                  context->return_utf8 );
		if( name != NULL ) {
			*attributes = (int)(context->data.dwFileAttributes & 0xFFu);
			return name;
		}
	}

	hCloseSearch( context );
	return NULL;
}

static char *hFindFirst( FB_DIRCTX *context, FBSTRING *filespec,
	                     int requested_attributes, int *attributes )
{
	wchar_t *wide_filespec;
	char *name;

	hCloseSearch( context );

	wide_filespec = fb_hConvertPathToWC( filespec->data,
	                                    &context->return_utf8 );
	if( wide_filespec == NULL )
		return NULL;

	context->handle = FindFirstFileW( wide_filespec, &context->data );
	free( wide_filespec );

	if( context->handle == INVALID_HANDLE_VALUE )
		return NULL;

	context->in_use = TRUE;
	context->allowed_attributes = (DWORD)requested_attributes | 0xFFFFFF00u;

	/* With ARCHIVE omitted, regular files remain eligible through DIRECTORY. */
	if( (requested_attributes & 0x10) == 0 )
		context->allowed_attributes |= 0x20u;

	if( !hAttributesAllowed( context ) )
		return hFindNext( context, attributes );

	name = fb_hConvertPathFromWC( context->data.cFileName,
	                              context->return_utf8 );
	if( name != NULL ) {
		*attributes = (int)(context->data.dwFileAttributes & 0xFFu);
		return name;
	}

	return hFindNext( context, attributes );
}

FBCALL FBSTRING *fb_Dir( FBSTRING *filespec, int attrib, int *out_attrib )
{
	FB_DIRCTX *context;
	FBSTRING *result;
	ssize_t filespec_length;
	ssize_t name_length;
	int temporary_attributes;
	char *name = NULL;

	if( out_attrib == NULL )
		out_attrib = &temporary_attributes;

	filespec_length = FB_STRSIZE( filespec );
	context = FB_TLSGETCTX( DIR );

	if( filespec_length > 0 )
		name = hFindFirst( context, filespec, attrib, out_attrib );
	else if( context->in_use )
		name = hFindNext( context, out_attrib );

	FB_STRLOCK();

	if( name != NULL ) {
		name_length = strlen( name );
		result = fb_hStrAllocTemp_NoLock( NULL, name_length );
		if( result != NULL )
			fb_hStrCopy( result->data, name, name_length );
		else
			result = &__fb_ctx.null_desc;

		free( name );
	} else {
		result = &__fb_ctx.null_desc;
		*out_attrib = 0;
	}

	/* Delete the descriptor when the caller supplied a temporary string. */
	fb_hStrDelTemp_NoLock( filespec );

	FB_STRUNLOCK();

	return result;
}

/* end of wince/file_dir.c */
