/*
 * Project: FreeBASIC runtime library
 * File: drivers/linux/sys_portio.c
 * Purpose: ports I/O functions.
 * Responsibilities: Target-specific runtime support for linux.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "../unix/fb_private_console.h"

int fb_hIn( unsigned short port )
{
#if defined HOST_X86 || defined HOST_X86_64
	unsigned char value;
	if (!__fb_con.has_perm)
		return -fb_ErrorSetNum( FB_RTERROR_NOPRIVILEGES );
	__asm__ volatile ("inb %1, %0" : "=a" (value) : "d" (port));
	return (int)value;
#else
	return -fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
#endif
}

int fb_hOut( unsigned short port, unsigned char value )
{
#if defined HOST_X86 || defined HOST_X86_64
	if (!__fb_con.has_perm)
		return fb_ErrorSetNum( FB_RTERROR_NOPRIVILEGES );
	__asm__ volatile ("outb %0, %1" : : "a" (value), "d" (port));
	return FB_RTERROR_OK;
#else
	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
#endif
}

/* end of drivers/linux/sys_portio.c */
