/*
    FreeBASIC Runtime Library
    File: ustr_paths.c
    Purpose: Adapt wide pathnames to the runtime's UTF-8 byte path interfaces.
    Responsibilities: Preserve Unicode and dispose of conversion temporaries.
    This file intentionally does NOT contain filesystem policy or enumeration.
*/

#include "fb.h"

/* Path APIs take terminated bytes. Unlike locale-based STRING conversion,
   these adapters keep all scalars and are independent of LC_CTYPE. The
   underlying platform provider continues to own the filesystem operation. */
static FBSTRING *hPath( const FB_WCHAR *path )
{
	FBSTRING *text;
	FB_STRLOCK();
	text = fb_hUStrFromWstr_NoLock(path);
	FB_STRUNLOCK();
	return text;
}

#define WIDE_PATH(name, result_type, operation, failure) \
FBCALL result_type name( const FB_WCHAR *path ) \
{ \
	FBSTRING *text = hPath(path); \
	if( text == NULL ) return failure; \
	result_type result = operation(text->data ? text->data : ""); \
	fb_hStrDelTemp(text); \
	return result; \
}

WIDE_PATH(fb_WideFileExists, int, fb_FileExists, FB_FALSE)
WIDE_PATH(fb_WideFileLen, long long, fb_FileLen, 0)
WIDE_PATH(fb_WideFileDateTime, double, fb_FileDateTime, 0.0)
WIDE_PATH(fb_WideFileGetAttr, int, fb_FileGetAttr, -1)

FBCALL int fb_WideFileSetAttr( const FB_WCHAR *path, int attributes )
{
	FBSTRING *text = hPath(path);
	int result;
	if( text == NULL ) return FB_RTERROR_OUTOFMEM;
	result = fb_FileSetAttr(text->data ? text->data : "", attributes);
	fb_hStrDelTemp(text);
	return result;
}

FBCALL int fb_WideFileCopy( const FB_WCHAR *source, const FB_WCHAR *destination )
{
	FBSTRING *src = hPath(source);
	FBSTRING *dst = hPath(destination);
	int result;
	if( src == NULL || dst == NULL ) result = FB_RTERROR_OUTOFMEM;
	else result = fb_FileCopy(src->data ? src->data : "", dst->data ? dst->data : "");
	fb_hStrDelTemp(src);
	fb_hStrDelTemp(dst);
	return result;
}

FBCALL int fb_WideFileCopyFromBytes( const char *source, const FB_WCHAR *destination )
{
	FBSTRING *dst = hPath(destination);
	int result;
	if( dst == NULL ) return FB_RTERROR_OUTOFMEM;
	result = fb_FileCopy(source, dst->data ? dst->data : "");
	fb_hStrDelTemp(dst);
	return result;
}

FBCALL int fb_WideFileCopyToBytes( const FB_WCHAR *source, const char *destination )
{
	FBSTRING *src = hPath(source);
	int result;
	if( src == NULL ) return FB_RTERROR_OUTOFMEM;
	result = fb_FileCopy(src->data ? src->data : "", destination);
	fb_hStrDelTemp(src);
	return result;
}

/* end of ustr_paths.c */
