/*
 * Project: FreeBASIC runtime library
 * File: hooks/hook_lineinp.c
 * Purpose: console line input function.
 * Responsibilities: Dispatch between console services and installed graphics hooks.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL int fb_LineInput
	(
		FBSTRING *text,
		void *dst,
		ssize_t dst_len,
		int fillrem,
		int addquestion,
		int addnewline
	)
{
    FB_LINEINPUTPROC lineinputproc;

    FB_LOCK();
    lineinputproc = __fb_ctx.hooks.lineinputproc;
    FB_UNLOCK();

    if( lineinputproc )
        return lineinputproc( text, dst, dst_len, fillrem, addquestion, addnewline );
    else
        return fb_ConsoleLineInput( text, dst, dst_len, fillrem, addquestion, addnewline );
}

/* end of hooks/hook_lineinp.c */
