/*
 * Project: FreeBASIC runtime library
 * File: console/con_lineinp.c
 * Purpose: console line input function.
 * Responsibilities: Console input, output, cursor state and PRINT formatting.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

static const char *pszDefaultQuestion = "? ";

// Neither version works correctly on android...

#if defined( HOST_WIN32 )

int fb_ConsoleLineInput
	(
		FBSTRING_ACCESS *text,
		void *dst,
		ssize_t dst_len,
		int fillrem,
		int addquestion,
		int addnewline
	)
{
    FBSTRING_ACCESS *tmp_result;

    FB_LOCK();

    fb_PrintBufferEx( NULL, 0, FB_PRINT_FORCE_ADJUST );

    if( text != NULL )
    {
        if( text->data != NULL )
        {
            fb_PrintString( 0, text, 0 );
        }
    	/* del if temp */
    	else
    	{
    		fb_hStrDelTemp( text );
    	}

        if( addquestion != FB_FALSE )
        {
            fb_PrintFixString( 0, pszDefaultQuestion, 0 );
        }
    }

    FB_UNLOCK();

    tmp_result = fb_ConReadLine( FALSE );

    if( addnewline ) {
				fb_PrintVoid( 0, FB_PRINT_NEWLINE );
    }

    if( tmp_result!=NULL ) {
        fb_StrAssign( dst, dst_len, tmp_result, -1, fillrem );
        return fb_ErrorSetNum( FB_RTERROR_OK );
    }

    return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
}

#else

static char *hWrapper( char *buffer, size_t count, FILE *fp )
{
    if( count > (size_t)FB_STRSIZEMSK )
        return NULL;
    return fb_ReadString( buffer, (ssize_t)count, fp );
}

int fb_ConsoleLineInput
	(
		FBSTRING_ACCESS *text,
		void *dst,
		ssize_t dst_len,
		int fillrem,
		int addquestion,
		int addnewline
	)
{
	int res, old_x;
	size_t len;
	ssize_t signed_len;

    fb_PrintBufferEx( NULL, 0, FB_PRINT_FORCE_ADJUST );
    fb_GetXY( &old_x, NULL );

	FB_LOCK();

    if( text != NULL )
    {
        if( text->data != NULL )
        {
            fb_PrintString( 0, text, 0 );
        }
    	/* del if temp */
    	else
    	{
    		fb_hStrDelTemp( text );
    	}

        if( addquestion != FB_FALSE )
        {
            fb_PrintFixString( 0, pszDefaultQuestion, 0 );
        }
    }

    {
        /* create temporary string */
        FBSTRING str_result = { .data = NULL, .len = 0, .size = 0 };

        res = fb_DevFileReadLineDumb( stdin, &str_result, hWrapper );

        signed_len = FB_STRSIZE(&str_result);
        len = (signed_len > 0) ? (size_t)signed_len : 0;

        /* We have to handle the NEWLINE stuff here because we *REQUIRE*
         * the *COMPLETE* temporary input string for the correct position
         * adjustment. */
        if( !addnewline ) {
            /* This is the easy and dumb method to do the position adjustment.
             * The problem is that it doesn't take TAB's into account. */
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


        /* add contents of tempporary string to result buffer */
        fb_StrAssign( dst, dst_len, (void *)&str_result, -1, fillrem );

        fb_StrDelete( &str_result );
    }

	FB_UNLOCK();

    return res;
}

#endif

/* end of console/con_lineinp.c */
