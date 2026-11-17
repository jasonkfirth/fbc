/*
 * Project: FreeBASIC runtime library
 * File: drivers/dos/io_getxy.c
 * Purpose: Provide io getxy runtime support.
 * Responsibilities: Target-specific runtime support for dos.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"
#include <go32.h>
#include <dpmi.h>
#include <sys/farptr.h>
#include <limits.h>

void fb_ConsoleGetXY_BIOS( int *col, int *row )
{
#if 0
    __dpmi_regs regs;
    regs.x.ax = 0x0300;
    regs.x.bx = fb_console_context.active;
    __dpmi_int(0x10, &regs);
    if( col!=NULL )
        *col = regs.h.dl;
    if( row!=NULL )
        *row = regs.h.dh;
#else
    unsigned long address;
    unsigned short usPos;
    int active = fb_console_context.active;

    if( active < 0 ||
        (unsigned long)active > (ULONG_MAX - 0x450ul) / sizeof(short) ) {
        if( col ) *col = -1;
        if( row ) *row = -1;
        return;
    }
    address = 0x450ul + (unsigned long)active * sizeof(short);
    usPos = _farpeekw( _dos_ds, address );
    if( col )
        *col = usPos & 0xFF;
    if( row )
        *row = (usPos >> 8) & 0xFF;
#endif
}

FBCALL void fb_ConsoleGetXY( int *col, int *row )
{
    fb_ConsoleGetXY_BIOS( col, row );
    if( col )
        ++*col;
    if( row )
        ++*row;
}

/* end of drivers/dos/io_getxy.c */
