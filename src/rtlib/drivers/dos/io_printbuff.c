/*
 * Project: FreeBASIC runtime library
 * File: drivers/dos/io_printbuff.c
 * Purpose: low-level print to console function.
 * Responsibilities: Target-specific runtime support for dos.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"
#include <unistd.h>
#include <go32.h>
#include <dpmi.h>
#include <limits.h>
#include <stdint.h>
#include <sys/farptr.h>

typedef struct FB_PRINT_INFO {
    unsigned short usAttr;
} fb_PrintInfo;

static void fb_hHookConScroll_BIOS
	(
		fb_ConHooks *handle,
		int x1, int y1,
		int x2, int y2,
		int rows
	)
{
    fb_ConsoleScroll_BIOS( x1, y1, x2, y2, rows );
    handle->Coord.Y = handle->Border.Bottom;
}

static int fb_hHookConWrite_BIOS
	(
		fb_ConHooks *handle,
		const void *buffer,
		size_t length
	)
{
    fb_PrintInfo *pInfo = (fb_PrintInfo*) handle->Opaque;
    const char *pachText = (const char *)buffer;
    __dpmi_regs regs;
    unsigned char uchAttr = (unsigned char) pInfo->usAttr;
    int pos = (handle->Coord.Y << 8) | handle->Coord.X;

    while( length-- ) {
        fb_hSetCursorPos( pos, -1 );
        ++pos;
        regs.h.ah = 0x09;
        regs.h.al = (unsigned char)*pachText++;
        regs.h.bh = (unsigned char)fb_console_context.active;
        regs.h.bl = uchAttr;
        regs.x.cx = 1;
        __dpmi_int(0x10, &regs);

    }

    return TRUE;
}

static int fb_hHookConWrite_MEM
	(
		fb_ConHooks *handle,
		const void *buffer,
		size_t length
	)
{
    fb_PrintInfo *pInfo = (fb_PrintInfo*) handle->Opaque;
    const char *pachText = (const char *)buffer;
    unsigned short usAttr = (unsigned short)(pInfo->usAttr << 8);
    unsigned short words[128];
    int tmp_x = handle->Coord.X;
    int tmp_y = handle->Coord.Y;
    unsigned long row_offset, column_offset, destination_offset;
    size_t written = 0;

    if ((!buffer && length) || (fb_console_context.w <= 0) ||
        (fb_console_context.h <= 0) || (tmp_x < 0) || (tmp_x >= fb_console_context.w) ||
        (tmp_y < 0) || (tmp_y >= fb_console_context.h))
        return FALSE;

    if ((tmp_y != 0) &&
        ((unsigned long)(unsigned int)fb_console_context.w >
         ULONG_MAX / 2UL / (unsigned long)(unsigned int)tmp_y))
        return FALSE;
    row_offset = (unsigned long)(unsigned int)fb_console_context.w * 2UL *
                 (unsigned long)(unsigned int)tmp_y;
    column_offset = (unsigned long)(unsigned int)tmp_x * 2UL;
    if ((row_offset > ULONG_MAX - fb_console_context.phys_addr) ||
        (column_offset > ULONG_MAX - fb_console_context.phys_addr - row_offset))
        return FALSE;
    destination_offset = fb_console_context.phys_addr + row_offset + column_offset;
    if ((unsigned long)length > (ULONG_MAX - destination_offset) / 2UL)
        return FALSE;

    while (written < length) {
        size_t count = length - written;
        size_t i;
        unsigned long chunk_offset = destination_offset + (unsigned long)(written * 2U);

        if (count > sizeof(words) / sizeof(words[0]))
            count = sizeof(words) / sizeof(words[0]);
        for (i = 0; i < count; i++) {
            words[i] = (unsigned short)(usAttr |
                (unsigned short)(unsigned char)pachText[written + i]);
        }

        _movedataw((unsigned)_my_ds(), (unsigned)(uintptr_t)words,
                   (unsigned)_dos_ds, (unsigned)chunk_offset, count);
        written += count;
    }

    return TRUE;
}

unsigned long fb_hGetPageAddr( int pg, int cols, int rows )
{
	unsigned long physAddr;
	unsigned long col_count, row_count, page_size, page_index;
    unsigned char mode = _farpeekb( _dos_ds, 0x465 );

    if (pg < 0)
        return 0;

    if( mode & 0x02 )
		return 0;

	if( mode & 0x04 )
    	physAddr = 0xB0000;
	else
    	physAddr = 0xB8000;

	if( pg != 0 ) {
		if( (cols == 0) || (rows == 0) )
			fb_ConsoleGetSize( &cols, &rows );

		if ((cols <= 0) || (rows <= 0))
			return 0;
		col_count = (unsigned long)(unsigned int)cols;
		row_count = (unsigned long)(unsigned int)rows;
		if ((row_count != 0) && (col_count > ULONG_MAX / 2UL / row_count))
			return 0;
		page_size = col_count * row_count * 2UL;
		if ((rows == 25) && (page_size > ULONG_MAX - 96UL))
			return 0;
		if (rows == 25)
			page_size += 96UL;
		page_index = (unsigned long)(unsigned int)pg;
		if ((page_size != 0) &&
		    (page_index > (ULONG_MAX - physAddr) / page_size))
			return 0;
		physAddr += page_index * page_size;
	}

	return physAddr;
}

static void fb_ConsolePrintBufferEx_SCRN
	(
		const void *buffer,
		size_t len,
		int mask
	)
{
    const char *pachText = (const char *) buffer;
    int win_left, win_top, win_cols, win_rows;
    int view_top, view_bottom;
    fb_PrintInfo info;
    fb_ConHooks hooks;

    /* Do we want to correct the console cursor position? */
    if( (mask & FB_PRINT_FORCE_ADJUST)==0 ) {
        /* No, we can check for the length to avoid unnecessary stuff ... */
        if( len==0 )
            return;
    }

    FB_LOCK();

    fb_ConsoleGetSize( &win_cols, &win_rows );
    fb_ConsoleGetView( &view_top, &view_bottom );
    win_left = win_top = 0;

    fb_console_context.w = win_cols;
    fb_console_context.h = win_rows;
    fb_console_context.phys_addr = fb_hGetPageAddr( fb_console_context.active, win_cols, win_rows );

    hooks.Opaque        = &info;
    if( fb_console_context.phys_addr ) {
        hooks.Scroll = fb_hHookConScroll_BIOS;
        hooks.Write  = fb_hHookConWrite_MEM;
    } else {
        hooks.Scroll = fb_hHookConScroll_BIOS;
        hooks.Write  = fb_hHookConWrite_BIOS;
    }
    hooks.Border.Left   = win_left;
    hooks.Border.Top    = win_top + view_top - 1;
    hooks.Border.Right  = win_left + win_cols - 1;
    hooks.Border.Bottom = win_top + view_bottom - 1;

    info.usAttr = (unsigned short) (unsigned char) fb_ConsoleGetColorAtt();

    {
        fb_ConsoleGetXY_BIOS( &hooks.Coord.X, &hooks.Coord.Y );

        if( fb_console_context.scrollWasOff ) {
            fb_console_context.scrollWasOff = FALSE;
            ++hooks.Coord.Y;
            hooks.Coord.X = hooks.Border.Left;
            fb_hConCheckScroll( &hooks );
        }

        fb_ConPrintTTY( &hooks, pachText, len, TRUE );

        if( hooks.Coord.X != hooks.Border.Left
            || hooks.Coord.Y != (hooks.Border.Bottom+1) )
        {
            fb_hConCheckScroll( &hooks );
        } else {
            fb_console_context.scrollWasOff = TRUE;
            hooks.Coord.X = hooks.Border.Right;
            hooks.Coord.Y = hooks.Border.Bottom;
        }
        fb_ConsoleLocate_BIOS( hooks.Coord.Y, hooks.Coord.X, -1 );
    }

    FB_UNLOCK();
}

static void fb_ConsolePrintBufferEx_STDIO
	(
		const void *buffer,
		size_t len,
		int mask
	)
{
	(void)mask;
    fwrite(buffer, len, 1, stdout);
    fflush(stdout);
}

void fb_ConsolePrintBufferEx( const void *buffer, size_t len, int mask )
{
	static void (*fn)(const void *buffer, size_t len, int mask) = NULL;

	if( fn == NULL ) {
		/* use cprintf() if STDOUT is the console;
       	   otherwise (with shell I/O redirection) use printf() */
    	if( isatty( fileno( stdout ) ) )
        	fn = fb_ConsolePrintBufferEx_SCRN;
    	else
        	fn = fb_ConsolePrintBufferEx_STDIO;
    }

    fn( buffer, len, mask );
}

void fb_ConsolePrintBuffer( const char *buffer, int mask )
{
	fb_ConsolePrintBufferEx( buffer, strlen( buffer ), mask );
}

/* end of drivers/dos/io_printbuff.c */
