/*
 * Project: FreeBASIC runtime library
 * File: console/io_printpad.c
 * Purpose: print functions.
 * Responsibilities: Console input, output, cursor state and PRINT formatting.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"
#include <limits.h>

static void fb_hPrintPadEx ( FB_FILE *handle, int mask, int current_x, int new_x )
{
#ifdef FB_NATIVE_TAB
	(void)mask;
    FB_PRINT_EX(handle, "\t", 1, mask);

#else
    char tab_char_buffer[FB_TAB_WIDTH+1];
	(void)mask;
    if (new_x <= current_x) {
        FB_PRINT_EX(handle, FB_NEWLINE, sizeof(FB_NEWLINE)-1, mask);
    } else {
        size_t count = (size_t)(new_x - current_x);
		if( count > FB_TAB_WIDTH )
			count = FB_TAB_WIDTH;
        memset(tab_char_buffer, 32, count);
        /* the terminating NUL shouldn't be required but it makes
         * debugging easier */
        tab_char_buffer[count] = 0;
        FB_PRINT_EX(handle, tab_char_buffer, count, mask);
    }
#endif
}

/*:::::*/
void fb_PrintPadEx ( FB_FILE *handle, int mask )
{
#ifdef FB_NATIVE_TAB
    FB_PRINT_EX(handle, "\t", 1, mask);

#else
    FB_FILE *tmp_handle;
   	int old_x;
    int new_x;

    fb_DevScrnInit_Write( );

    tmp_handle = FB_HANDLE_DEREF(handle);
    if( tmp_handle == NULL ) {
        return;
    }
    if( tmp_handle->line_length > (unsigned int)(INT_MAX - FB_TAB_WIDTH) ) {
        fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
        return;
    }

    old_x = (int)tmp_handle->line_length + 1;
    new_x = old_x + FB_TAB_WIDTH - 1;
    new_x /= FB_TAB_WIDTH;
    new_x *= FB_TAB_WIDTH;
    new_x += 1;
    if (tmp_handle->width!=0) {
        /* If padding moved us beyond EOL, move to beginning of next line */
        if ((unsigned int)new_x > tmp_handle->width) {
            new_x = 1;
        }
    }
    fb_hPrintPadEx(handle, mask, old_x, new_x);
#endif
}

/*:::::*/
FBCALL void fb_PrintPad ( int fnum, int mask )
{
    fb_PrintPadEx( FB_FILE_TO_HANDLE(fnum), mask );
}

/* end of console/io_printpad.c */
