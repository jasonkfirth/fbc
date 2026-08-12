/*
    DOS console line-input implementation

    This complete target file is selected by basename precedence.  Keep DOS
    behavior here instead of adding HOST_DOS branches to the shared source.
*/

/* console line input function */

#include "fb.h"

static const char *pszDefaultQuestion = "? ";

// Neither version works correctly on android...

int fb_ConsoleLineInput
	(
		FBSTRING *text,
		void *dst,
		ssize_t dst_len,
		int fillrem,
		int addquestion,
		int addnewline
	)
{
    FBSTRING *tmp_result;

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
