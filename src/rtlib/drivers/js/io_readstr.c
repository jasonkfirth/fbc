/*
 * Project: FreeBASIC runtime library
 * File: drivers/js/io_readstr.c
 * Purpose: console line input function.
 * Responsibilities: Target-specific runtime support for js.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"


char *fb_ConsoleReadStr( char *buffer, ssize_t len )
{
	EM_ASM_ARGS({
		var jsString = __fb_rtlib.console.input();
		jsString+='\n';
		stringToUTF8(jsString, $0, $1);
	}, buffer, len );


	return buffer;

	//return fgets( buffer, len, stdin );
}

/* end of drivers/js/io_readstr.c */
