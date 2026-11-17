/*
 * Project: FreeBASIC runtime library
 * File: drivers/dos/io_scroll.c
 * Purpose: console scrolling for when VIEW is used.
 * Responsibilities: Target-specific runtime support for dos.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"
#include <dpmi.h>
#include <go32.h>
#include <limits.h>
#include <stdint.h>

static int hGetScreenSize( int *cols, int *rows )
{
	fb_ConsoleGetSize( cols, rows );
	return (*cols > 0) && (*rows > 0) &&
	       (*cols <= UCHAR_MAX + 1) && (*rows <= UCHAR_MAX + 1);
}

static int hRectIsValid( int x1, int y1, int x2, int y2,
	                     int cols, int rows )
{
	return (x1 >= 0) && (y1 >= 0) && (x2 >= x1) && (y2 >= y1) &&
	       (x2 < cols) && (y2 < rows);
}

static int hGetPageWordAddress( unsigned long page_addr, int cols,
	                            int x, int y, unsigned long *address )
{
	unsigned long word_offset;
	unsigned long byte_offset;

	word_offset = (unsigned long)(unsigned int)y * (unsigned long)(unsigned int)cols +
	              (unsigned long)(unsigned int)x;
	byte_offset = word_offset * (unsigned long)sizeof( unsigned short );
	if( page_addr > ULONG_MAX - byte_offset )
		return FALSE;

	*address = page_addr + byte_offset;
	return TRUE;
}

static void hFillRect( int x1, int y1, int x2, int y2 )
{
	int cols, rows, row;
	unsigned int color;
	unsigned int width, i;
	unsigned long page_addr;
	unsigned short buffer[UCHAR_MAX + 1];
	unsigned short attr;

	if( !hGetScreenSize( &cols, &rows ) ||
	    !hRectIsValid( x1, y1, x2, y2, cols, rows ) )
		return;

	width = (unsigned int)(x2 - x1 + 1);
	color = fb_ConsoleGetColorAtt();
	if( color > UCHAR_MAX )
		return;
	attr = (unsigned short)(color << 8);

	page_addr = fb_hGetPageAddr( fb_console_context.active, cols, rows );
	if( page_addr == 0 )
	{
		/* BIOS character output is limited to one row's 16-bit count. */
		unsigned short old_pos = fb_hSetCursorPos( x1, y1 );

		__dpmi_regs regs;
		for( row = y1; row <= y2; row++ ) {
			if( row != y1 )
				fb_hSetCursorPos( x1, row );
			regs.x.ax = 0x0920;
			regs.h.bh = (unsigned char)fb_console_context.active;
			regs.h.bl = (unsigned char)color;
			regs.x.cx = (unsigned short)width;
			__dpmi_int(0x10, &regs);
		}

		fb_hSetCursorPos( old_pos, -1 );
	}
	else
	{
		for( i = 0; i < width; i++ )
			buffer[i] = attr | 0x20;

		for( row = y1; row <= y2; row++ ) {
			unsigned long dest_addr;
			if( !hGetPageWordAddress( page_addr, cols, x1, row, &dest_addr ) )
				return;
			_movedataw( (unsigned int)_my_ds(), (unsigned int)(uintptr_t)buffer,
			            _dos_ds, dest_addr, (unsigned long)width );
		}
	}
}

static void hMoveRect( int x1, int y1, int x2, int y2, int nrows )
{
	int cols, rows;
	unsigned int width, height, move_rows, rows_to_move, i;
	unsigned long page_addr;

	if( !hGetScreenSize( &cols, &rows ) ||
	    !hRectIsValid( x1, y1, x2, y2, cols, rows ) || (nrows == 0) )
		return;

	width = (unsigned int)(x2 - x1 + 1);
	height = (unsigned int)(y2 - y1 + 1);
	if( nrows < 0 )
		rows_to_move = (unsigned int)(-(nrows + 1)) + 1U;
	else
		rows_to_move = (unsigned int)nrows;
	if( rows_to_move >= height )
		return;

	page_addr = fb_hGetPageAddr( fb_console_context.active, cols, rows );
	if( page_addr == 0 )
	{
		/*
			This is an off-screen page without a direct segment address.  The BIOS
			scroll call operates on the visible text page, so there is no safe DOS
			fallback for this case.
		*/
	}
	else
	{
		move_rows = height - rows_to_move;
		if( nrows > 0 ) {
			for( i = 0; i < move_rows; i++ ) {
				unsigned long src_addr, dest_addr;
				int src_row = y1 + (int)rows_to_move + (int)i;
				int dest_row = y1 + (int)i;
				if( !hGetPageWordAddress( page_addr, cols, x1, src_row, &src_addr ) ||
				    !hGetPageWordAddress( page_addr, cols, x1, dest_row, &dest_addr ) )
					return;
				_movedataw( _dos_ds, src_addr, _dos_ds, dest_addr,
				            (unsigned long)width );
			}
		}
		else {
			for( i = move_rows; i > 0; i-- ) {
				unsigned long src_addr, dest_addr;
				int src_row = y1 + (int)(i - 1U);
				int dest_row = src_row + (int)rows_to_move;
				if( !hGetPageWordAddress( page_addr, cols, x1, src_row, &src_addr ) ||
				    !hGetPageWordAddress( page_addr, cols, x1, dest_row, &dest_addr ) )
					return;
				_movedataw( _dos_ds, src_addr, _dos_ds, dest_addr,
				            (unsigned long)width );
			}
		}
	}
}

void fb_ConsoleScroll_BIOS( int x1, int y1, int x2, int y2, int nrows )
{
	int cols, rows;
	unsigned int color;
	unsigned int scroll_rows;
	unsigned int rect_height;
	__dpmi_regs regs;

	if( !hGetScreenSize( &cols, &rows ) ||
	    !hRectIsValid( x1, y1, x2, y2, cols, rows ) )
		return;
	if( nrows < 0 )
		scroll_rows = (unsigned int)(-(nrows + 1)) + 1U;
	else
		scroll_rows = (unsigned int)nrows;
	if( scroll_rows > UCHAR_MAX )
		return;
	rect_height = (unsigned int)(y2 - y1 + 1);
	if( scroll_rows >= rect_height ) {
		nrows = 0;
		scroll_rows = 0;
	}

	/* The BIOS cannot scroll a non-visible page. */
	if( fb_console_context.active != fb_console_context.visible )
	{
		if( fb_hGetPageAddr( fb_console_context.active, cols, rows ) == 0 )
			return;
		if( nrows == 0 )
		{
			hFillRect( x1, y1, x2, y2 );
		}
		else
		{
			hMoveRect( x1, y1, x2, y2, nrows );
			if( nrows > 0 )
				hFillRect( x1, y2 - (int)scroll_rows + 1, x2, y2 );
			else
				hFillRect( x1, y1, x2, y1 + (int)scroll_rows - 1 );
		}

		return;
	}

	color = fb_ConsoleGetColorAtt();
	if( color > UCHAR_MAX )
		return;
	regs.h.bl = 0;
	regs.h.bh = (unsigned char)color;
	regs.h.cl = (unsigned char)x1;
	regs.h.ch = (unsigned char)y1;
	regs.h.dl = (unsigned char)x2;
	regs.h.dh = (unsigned char)y2;
	if( nrows >= 0 )
		regs.x.ax = (unsigned short)(0x0600U | scroll_rows);
	else
		regs.x.ax = (unsigned short)(0x0700U | scroll_rows);
	__dpmi_int(0x10, &regs);
}

void fb_ConsoleScrollEx( int x1, int y1, int x2, int y2, int nrows )
{
	int cols, rows;
	unsigned int scroll_rows;

	if( !hGetScreenSize( &cols, &rows ) ||
	    (x1 < 1) || (y1 < 1) || (x2 < x1) || (y2 < y1) ||
	    (x2 > cols) || (y2 > rows) )
		return;

	if( nrows < 0 )
		scroll_rows = (unsigned int)(-(nrows + 1)) + 1U;
	else
		scroll_rows = (unsigned int)nrows;
	if( scroll_rows >= (unsigned int)rows )
        nrows = 0;
	if( (nrows == 0) && (scroll_rows == 0) )
		return;

	fb_ConsoleScroll_BIOS( x1 - 1, y1 - 1, x2 - 1, y2 - 1, nrows );

#if 0
    if( nrows > 0 ) {
        fb_ConsoleLocate_BIOS( y2 - 1 - nrows, x1, -1 );
    } else if( nrows < 0 ) {
        fb_ConsoleLocate_BIOS( y1 - 1 - nrows, x1, -1 );
    } else {
        fb_ConsoleLocate_BIOS( y1 - 1, x1 - 1, -1 );
    }
#endif
}

void fb_ConsoleScroll( int nrows )
{
	int toprow, botrow;
    int cols;

	if( nrows == 0 )
        return;

	fb_ConsoleGetSize( &cols, NULL );
    fb_ConsoleGetView( &toprow, &botrow );

	fb_ConsoleScrollEx( 1, toprow, cols, botrow, nrows );
}

/* end of drivers/dos/io_scroll.c */
