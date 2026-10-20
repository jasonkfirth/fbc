/*
 * Project: FreeBASIC runtime library
 * File: drivers/dos/sys_portio.c
 * Purpose: ports I/O functions.
 * Responsibilities: Target-specific runtime support for dos.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

int fb_hIn( unsigned short port )
{
	unsigned char value;
	__asm__ volatile ("inb %1, %0" : "=a" (value) : "d" (port));
	return (int)value;
}

int fb_hOut( unsigned short port, unsigned char value )
{
	__asm__ volatile ("outb %0, %1" : : "a" (value), "d" (port));
	return FB_RTERROR_OK;
}

/* end of drivers/dos/sys_portio.c */
