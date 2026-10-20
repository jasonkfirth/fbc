/*
 * Project: FreeBASIC runtime library
 * File: hooks/hook_lineinp_wstr.c
 * Purpose: console line input function.
 * Responsibilities: Dispatch between console services and installed graphics hooks.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL int fb_LineInputWstr
	(
		const FB_WCHAR *text,
		FB_WCHAR *dst,
		ssize_t max_chars,
		int addquestion,
		int addnewline
	)
{
    FB_LINEINPUTWPROC fn;

    FB_LOCK();
    fn = __fb_ctx.hooks.lineinputwproc;
    FB_UNLOCK();

    if( fn )
        return fn( text, dst, max_chars, addquestion, addnewline );
    else
        return fb_ConsoleLineInputWstr( text, dst, max_chars, addquestion, addnewline );
}

/* end of hooks/hook_lineinp_wstr.c */
