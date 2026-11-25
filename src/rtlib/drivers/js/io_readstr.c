/*
 * Project: FreeBASIC runtime library
 * File: drivers/js/io_readstr.c
 * Purpose: console line input function.
 * Responsibilities: Target-specific runtime support for js.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <emscripten/emscripten.h>

/* INPUT is synchronous to BASIC. Asyncify suspends the call while an HTML
 * shell with fbReadLine waits for input, so the browser can paint the prompt
 * and process the input form without blocking the rest of the page. */
EM_ASYNC_JS( int, fb_js_console_read, (char *buffer, int capacity), {
	var bytes = Module['fbConsoleInputBytes'];
	if (!bytes) {
		var line;
		if (typeof Module['fbReadLine'] === 'function')
			line = await Module['fbReadLine']();
		else if (typeof prompt === 'function')
			line = __fb_rtlib.console.input();
		if (typeof line !== 'string')
			return 0;
		bytes = new TextEncoder().encode(line + '\n');
	}

	// FreeBASIC assembles input in fgets-sized chunks. Retain the remaining
	// bytes, including the final LF, instead of prompting for a second line.
	var count = Math.min(bytes.length, capacity - 1);
	HEAPU8.set(bytes.subarray(0, count), buffer);
	HEAPU8[buffer + count] = 0;
	Module['fbConsoleInputBytes'] = count < bytes.length ? bytes.subarray(count) : null;
	return 1;
});

char *fb_ConsoleReadStr( char *buffer, ssize_t len )
{
	if( buffer == NULL || len <= 1 )
		return NULL;
	return fb_js_console_read( buffer, len ) ? buffer : NULL;
}

/* end of drivers/js/io_readstr.c */
