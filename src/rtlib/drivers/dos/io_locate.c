/*
 * Project: FreeBASIC runtime library
 * File: drivers/dos/io_locate.c
 * Purpose: console LOCATE statement.
 * Responsibilities: Target-specific runtime support for dos.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"
#include <go32.h>
#include <dpmi.h>
#include <sys/farptr.h>

static unsigned short fb_hGetConsolePage(void)
{
	int active = fb_console_context.active;

	if ((active < 0) || (active > 7))
		return 0;
	return (unsigned short)active;
}

static unsigned long fb_hGetCursorPositionAddress(void)
{
	return 0x450UL + (unsigned long)fb_hGetConsolePage() *
	       (unsigned long)sizeof(unsigned short);
}

static unsigned short fb_hPackCursorPosition(int col, int row)
{
	return (unsigned short)(((unsigned int)(unsigned char)row << 8) |
	                        (unsigned int)(unsigned char)col);
}

/* Raw DOS console cursor positioning function, faster than console LOCATE */
unsigned short fb_hSetCursorPos( int col, int row )
{
	unsigned long addr = fb_hGetCursorPositionAddress();

	if( row >= 0 ) {
		unsigned short old = _farpeekw( _dos_ds, addr );
		_farpokew( _dos_ds, addr, fb_hPackCursorPosition(col, row) );
		return old;
	} else {
		_farpokew( _dos_ds, addr, (unsigned short)col );
		return 0;
	}
}

int fb_ConsoleLocate_BIOS( int row, int col, int cursor )
{
    __dpmi_regs regs;
    int x, y;
    int shape_visible;
    unsigned short usShapePos, usShapeSize;

    usShapePos = _farpeekw( _dos_ds, fb_hGetCursorPositionAddress() );
    usShapeSize = _farpeekw( _dos_ds, 0x460 );
    shape_visible = (usShapeSize & 0xC000)==0x0000;

    if( col >= 0 )
        x = col;
    else
        x = usShapePos & 0xFF;

    if( row >= 0 )
        y = row;
    else
        y = (usShapePos >> 8) & 0xFF;

    /*
    	Keep the BIOS cursor position synchronized before changing the visible
    	hardware cursor shape.  Some DOS targets use the BIOS cursor state as
    	the authoritative location when pages are switched.
    */
    regs.x.ax = 0x0200;
    regs.x.bx = fb_hGetConsolePage();
    regs.h.dh = (unsigned char) y;
    regs.h.dl = (unsigned char) x;
    __dpmi_int(0x10, &regs);

    _farpokew( _dos_ds, fb_hGetCursorPositionAddress(),
               fb_hPackCursorPosition(x, y) );

    if( cursor >= 0)
    {
        int shape_start, shape_end;

        shape_start = (usShapeSize >> 8) & 0x1F;
        shape_end = usShapeSize & 0x1F;
        shape_visible = cursor!=0;

        regs.x.ax = 0x0100;
        regs.h.ch = (unsigned char) (shape_start + (shape_visible ? 0x00 : 0x20));
        regs.h.cl = (unsigned char) shape_end;
        __dpmi_int(0x10, &regs);
    }

    return ((int)(unsigned char)x | ((int)(unsigned char)y << 8) |
            (shape_visible ? 0x10000 : 0));
}

int fb_ConsoleLocate( int row, int col, int cursor )
{
    int bios_row = row > 0 ? row - 1 : -1;
    int bios_col = col > 0 ? col - 1 : -1;
    int result = fb_ConsoleLocate_BIOS( bios_row, bios_col, cursor );
    fb_console_context.scrollWasOff = FALSE;
    return result + 0x0101;
}

/* end of drivers/dos/io_locate.c */
