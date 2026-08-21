/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/console_state.c

    Purpose:

        Maintain Windows CE console text independently of desktop console APIs.

    Responsibilities:

        - allocate and resize up to four logical text pages
        - retain characters and color attributes for SCREEN and PCOPY
        - apply cursor, wrapping, view-region, and scrolling semantics

    This file intentionally does NOT contain:

        - native window creation or painting
        - keyboard and pointer input
        - standard-stream transport

    Windows CE console model:

        Windows CE does not implement the Win32 console screen-buffer API.
        The runtime therefore keeps the observable BASIC text-console state in
        memory.  Standard output remains the transport for sequential text,
        while this model supplies the random-access behavior required by
        LOCATE, SCREEN, VIEW, CLS, WIDTH, SCREEN pages, and PCOPY.
*/

#include "../fb.h"
#include "fb_private_console.h"

#include <limits.h>

/* ------------------------------------------------------------------------- */
/* Allocation helpers                                                        */
/* ------------------------------------------------------------------------- */

static size_t hCellCount( int columns, int rows )
{
	if( (columns <= 0) || (rows <= 0) )
		return 0;

	if( (size_t)columns > (SIZE_MAX / (size_t)rows) )
		return 0;

	return (size_t)columns * (size_t)rows;
}

static void hFillCells( FB_WINCE_CONSOLE_CELL *cells, size_t count )
{
	size_t index;
	unsigned char attribute;

	attribute = (unsigned char)(__fb_con.foreground |
	                            (__fb_con.background << 4));
	for( index = 0; index < count; ++index ) {
		cells[index].character = (FB_WCHAR)' ';
		cells[index].attribute = attribute;
	}
}

int fb_hWinCEConsoleEnsurePage( int page )
{
	size_t count;

	if( (page < 0) || (page >= FB_CONSOLE_MAXPAGES) )
		return FALSE;

	if( __fb_con.pages[page].cells != NULL )
		return TRUE;

	count = hCellCount( __fb_con.columns, __fb_con.rows );
	if( count == 0 )
		return FALSE;

	__fb_con.pages[page].cells =
		malloc( count * sizeof(FB_WINCE_CONSOLE_CELL) );
	if( __fb_con.pages[page].cells == NULL )
		return FALSE;

	hFillCells( __fb_con.pages[page].cells, count );
	return TRUE;
}

int fb_hWinCEConsoleInitialize( void )
{
	__fb_con.columns = FB_SCRN_DEFAULT_WIDTH;
	__fb_con.rows = FB_SCRN_DEFAULT_HEIGHT;
	__fb_con.cursor_visible = TRUE;
	__fb_con.foreground = FB_COLOR_WHITE;
	__fb_con.background = FB_COLOR_BLACK;

	return fb_hWinCEConsoleEnsurePage( 0 );
}

void fb_hWinCEConsoleShutdown( void )
{
	int page;

	for( page = 0; page < FB_CONSOLE_MAXPAGES; ++page ) {
		free( __fb_con.pages[page].cells );
		__fb_con.pages[page].cells = NULL;
	}
}

int fb_hWinCEConsoleResize( int columns, int rows )
{
	FB_WINCE_CONSOLE_CELL *replacement[FB_CONSOLE_MAXPAGES] = { NULL };
	size_t count;
	int copy_columns;
	int copy_rows;
	int page;
	int row;

	count = hCellCount( columns, rows );
	if( count == 0 )
		return FALSE;

	copy_columns = (columns < __fb_con.columns) ? columns : __fb_con.columns;
	copy_rows = (rows < __fb_con.rows) ? rows : __fb_con.rows;

	for( page = 0; page < FB_CONSOLE_MAXPAGES; ++page ) {
		if( __fb_con.pages[page].cells == NULL )
			continue;

		replacement[page] =
			malloc( count * sizeof(FB_WINCE_CONSOLE_CELL) );
		if( replacement[page] == NULL )
			goto allocation_failed;

		hFillCells( replacement[page], count );
		for( row = 0; row < copy_rows; ++row ) {
			memcpy( replacement[page] + ((size_t)row * columns),
			        __fb_con.pages[page].cells +
			            ((size_t)row * __fb_con.columns),
			        (size_t)copy_columns * sizeof(FB_WINCE_CONSOLE_CELL) );
		}
	}

	for( page = 0; page < FB_CONSOLE_MAXPAGES; ++page ) {
		if( replacement[page] == NULL )
			continue;

		free( __fb_con.pages[page].cells );
		__fb_con.pages[page].cells = replacement[page];
	}

	__fb_con.columns = columns;
	__fb_con.rows = rows;
	if( __fb_con.cursor_x >= columns )
		__fb_con.cursor_x = columns - 1;
	if( __fb_con.cursor_y >= rows )
		__fb_con.cursor_y = rows - 1;

	return TRUE;

allocation_failed:
	for( page = 0; page < FB_CONSOLE_MAXPAGES; ++page )
		free( replacement[page] );

	return FALSE;
}

/* ------------------------------------------------------------------------- */
/* Page access and region operations                                         */
/* ------------------------------------------------------------------------- */

FB_WINCE_CONSOLE_CELL *fb_hWinCEConsoleCell( int page, int column, int row )
{
	if( !fb_hWinCEConsoleEnsurePage( page ) )
		return NULL;

	if( (column < 0) || (column >= __fb_con.columns) ||
	    (row < 0) || (row >= __fb_con.rows) )
		return NULL;

	return __fb_con.pages[page].cells +
	       ((size_t)row * __fb_con.columns) + column;
}

static int hClipRegion( int *left, int *top, int *right, int *bottom )
{
	if( *left < 0 )
		*left = 0;
	if( *top < 0 )
		*top = 0;
	if( *right >= __fb_con.columns )
		*right = __fb_con.columns - 1;
	if( *bottom >= __fb_con.rows )
		*bottom = __fb_con.rows - 1;

	return (*left <= *right) && (*top <= *bottom);
}

void fb_hWinCEConsoleClearRegion( int page, int left, int top, int right,
	                              int bottom )
{
	FB_WINCE_CONSOLE_CELL *cell;
	unsigned char attribute;
	int column;
	int row;

	if( !hClipRegion( &left, &top, &right, &bottom ) )
		return;

	attribute = (unsigned char)(__fb_con.foreground |
	                            (__fb_con.background << 4));
	for( row = top; row <= bottom; ++row ) {
		for( column = left; column <= right; ++column ) {
			cell = fb_hWinCEConsoleCell( page, column, row );
			if( cell == NULL )
				return;
			cell->character = (FB_WCHAR)' ';
			cell->attribute = attribute;
		}
	}
}

void fb_hWinCEConsoleScrollRegion( int page, int left, int top, int right,
	                               int bottom, int rows )
{
	FB_WINCE_CONSOLE_CELL *cells;
	size_t line_bytes;
	int row;

	if( (rows <= 0) ||
	    !hClipRegion( &left, &top, &right, &bottom ) ||
	    !fb_hWinCEConsoleEnsurePage( page ) )
		return;

	if( rows > (bottom - top) ) {
		fb_hWinCEConsoleClearRegion( page, left, top, right, bottom );
		return;
	}

	cells = __fb_con.pages[page].cells;
	line_bytes = (size_t)(right - left + 1) *
	             sizeof(FB_WINCE_CONSOLE_CELL);
	for( row = top; row <= (bottom - rows); ++row ) {
		memmove( cells + ((size_t)row * __fb_con.columns) + left,
		         cells + ((size_t)(row + rows) * __fb_con.columns) + left,
		         line_bytes );
	}

	fb_hWinCEConsoleClearRegion( page, left, bottom - rows + 1, right,
	                            bottom );
}

/* ------------------------------------------------------------------------- */
/* Sequential text updates                                                   */
/* ------------------------------------------------------------------------- */

static void hCheckCursor( void )
{
	int top;
	int bottom;

	top = fb_ConsoleGetTopRow();
	bottom = fb_ConsoleGetBotRow();
	if( top < 0 )
		top = 0;
	if( bottom >= __fb_con.rows )
		bottom = __fb_con.rows - 1;

	if( __fb_con.cursor_x >= __fb_con.columns ) {
		__fb_con.cursor_x = 0;
		++__fb_con.cursor_y;
	}

	if( __fb_con.cursor_y > bottom ) {
		fb_hWinCEConsoleScrollRegion( __fb_con.active, 0, top,
		                            __fb_con.columns - 1, bottom, 1 );
		__fb_con.cursor_y = bottom;
	}
}

static void hWriteCharacter( unsigned char character )
{
	FB_WINCE_CONSOLE_CELL *cell;
	int spaces;

	switch( character ) {
	case '\r':
		__fb_con.cursor_x = 0;
		return;

	case '\n':
		__fb_con.cursor_x = 0;
		++__fb_con.cursor_y;
		hCheckCursor();
		return;

	case '\b':
		if( __fb_con.cursor_x > 0 )
			--__fb_con.cursor_x;
		return;

	case '\t':
		spaces = FB_TAB_WIDTH - (__fb_con.cursor_x % FB_TAB_WIDTH);
		while( spaces-- > 0 )
			hWriteCharacter( (unsigned char)' ' );
		return;

	default:
		if( character < 32 )
			return;
		break;
	}

	cell = fb_hWinCEConsoleCell( __fb_con.active, __fb_con.cursor_x,
	                            __fb_con.cursor_y );
	if( cell != NULL ) {
		cell->character = (FB_WCHAR)character;
		cell->attribute =
			(unsigned char)(__fb_con.foreground |
			                (__fb_con.background << 4));
	}

	++__fb_con.cursor_x;
	hCheckCursor();
}

void fb_hWinCEConsoleWrite( const char *text, size_t length )
{
	size_t index;

	for( index = 0; index < length; ++index )
		hWriteCharacter( (unsigned char)text[index] );
}

/* end of wince/console_state.c */
