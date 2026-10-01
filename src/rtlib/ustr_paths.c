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
#define WIDE_PATH(name, result_type, operation) \
FBCALL result_type name( const FB_WCHAR *path ) \
{ \
	FBSTRING *text = fb_UStrFromWstr(path); \
	result_type result = operation(text->data ? text->data : ""); \
	fb_hStrDelTemp(text); \
	return result; \
}

WIDE_PATH(fb_WideFileExists, int, fb_FileExists)
WIDE_PATH(fb_WideFileLen, long long, fb_FileLen)
WIDE_PATH(fb_WideFileDateTime, double, fb_FileDateTime)
WIDE_PATH(fb_WideFileGetAttr, int, fb_FileGetAttr)

FBCALL int fb_WideFileSetAttr( const FB_WCHAR *path, int attributes )
{
	FBSTRING *text = fb_UStrFromWstr(path);
	int result = fb_FileSetAttr(text->data ? text->data : "", attributes);
	fb_hStrDelTemp(text);
	return result;
}

FBCALL int fb_WideFileCopy( const FB_WCHAR *source, const FB_WCHAR *destination )
{
	FBSTRING *src = fb_UStrFromWstr(source);
	FBSTRING *dst = fb_UStrFromWstr(destination);
	int result = fb_FileCopy(src->data ? src->data : "", dst->data ? dst->data : "");
	fb_hStrDelTemp(src);
	fb_hStrDelTemp(dst);
	return result;
}

FBCALL int fb_WideFileCopyFromBytes( const char *source, const FB_WCHAR *destination )
{
	FBSTRING *dst = fb_UStrFromWstr(destination);
	int result = fb_FileCopy(source, dst->data ? dst->data : "");
	fb_hStrDelTemp(dst);
	return result;
}

FBCALL int fb_WideFileCopyToBytes( const FB_WCHAR *source, const char *destination )
{
	FBSTRING *src = fb_UStrFromWstr(source);
	int result = fb_FileCopy(src->data ? src->data : "", destination);
	fb_hStrDelTemp(src);
	return result;
}

/* end of ustr_paths.c */
