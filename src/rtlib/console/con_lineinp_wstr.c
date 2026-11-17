/*
 * Project: FreeBASIC runtime library
 * File: console/con_lineinp_wstr.c
 * Purpose: console line input function for wstrings.
 * Responsibilities: Console input, output, cursor state and PRINT formatting.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

static const char *pszDefaultQuestion = "? ";

#if defined( HOST_WIN32 )

int fb_ConsoleLineInputWstr
	(
		const FB_WCHAR *text,
		FB_WCHAR *dst,
		ssize_t max_chars,
		int addquestion,
		int addnewline
	)
{
    FBSTRING_ACCESS *tmp_result;

    /* !!!FIXME!!! no support for unicode input */

    FB_LOCK();

    fb_PrintBufferEx( NULL, 0, FB_PRINT_FORCE_ADJUST );

    if( text != NULL )
    {
        fb_PrintWstr( 0, text, 0 );

        if( addquestion != FB_FALSE )
            fb_PrintFixString( 0, pszDefaultQuestion, 0 );
    }

    FB_UNLOCK();

    tmp_result = fb_ConReadLine( FALSE );

    if( addnewline )
      fb_PrintVoid( 0, FB_PRINT_NEWLINE );

    if( tmp_result == NULL )
        return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );

    fb_WstrAssignFromA( dst, max_chars, tmp_result, -1 );

    return fb_ErrorSetNum( FB_RTERROR_OK );
}

#else

static char *hWrapper( char *buffer, size_t count, FILE *fp )
{
    if( count > (size_t)FB_STRSIZEMSK )
        return NULL;
    return fb_ReadString( buffer, (ssize_t)count, fp );
}

int fb_ConsoleLineInputWstr
	(
		const FB_WCHAR *text,
		FB_WCHAR *dst,
		ssize_t max_chars,
		int addquestion,
		int addnewline
	)
{
	size_t len;
	int res, old_x;

    /* !!!FIXME!!! no support for unicode input */

    fb_PrintBufferEx( NULL, 0, FB_PRINT_FORCE_ADJUST );
    fb_GetXY( &old_x, NULL );

	FB_LOCK();

    if( text != NULL )
    {
		fb_PrintWstr( 0, text, 0 );

        if( addquestion != FB_FALSE )
            fb_PrintFixString( 0, pszDefaultQuestion, 0 );
    }

    {
        FBSTRING str_result = { 0, 0, 0 };

        res = fb_DevFileReadLineDumb( stdin, &str_result, hWrapper );

        len = FB_STRSIZE(&str_result);

        if( !addnewline )
        {
            int cols, rows, old_y;

            fb_GetSize( &cols, &rows );
            fb_GetXY( NULL, &old_y );

            if( cols > 0 && old_x > 0 && old_y > 0 ) {
                size_t columns = (size_t)cols;
                size_t column = (size_t)(old_x - 1) % columns;
                column = (column + len % columns) % columns;
                fb_Locate( old_y - 1, (int)column + 1, -1, 0, 0 );
            }
        }

        fb_WstrAssignFromA( dst, max_chars, (void *)&str_result, -1 );

        fb_StrDelete( &str_result );
    }

	FB_UNLOCK();

    return res;
}

#endif

/* end of console/con_lineinp_wstr.c */
