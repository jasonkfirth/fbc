/*
 * Project: FreeBASIC runtime library
 * File: console/io_viewhlp.c
 * Purpose: Store and bound the VIEW PRINT text-row range.
 * Responsibilities: Preserve console caching and follow graphics text resizing.
 * This file intentionally does NOT contain: rendering or native window resizing.
 */
#include "fb.h"

static int view_toprow = -1, view_botrow = -1;
static int view_rows = -1;

/*
 * A compositor can resize a graphics mode without another SCREEN statement.
 * Keep a bottom-anchored range at the new bottom and clip explicit ranges.
 * Otherwise CLS/PRINT can address rows beyond the resized framebuffer.
 *
 * Callers retain their existing runtime/graphics synchronization. Query the
 * installed size hook directly, without taking FB_LOCK after a graphics lock.
 * Do not query the native console here: its cached path is also used during
 * terminal shutdown, where another terminal-size request may be unsafe.
 */
static void fb_hSyncGraphicsView( void )
{
    int rows = 0;

    if( fb_ctx.hooks.getsizeproc == NULL )
        return;
    fb_ctx.hooks.getsizeproc( NULL, &rows );
    if( rows <= 0 || rows == view_rows )
        return;

    if( view_botrow < 0 || view_botrow == view_rows - 1 || view_botrow >= rows )
        view_botrow = rows - 1;
    if( view_toprow < 0 )
        view_toprow = 0;
    if( view_toprow > view_botrow )
        view_toprow = view_botrow;
    view_rows = rows;
}

/*:::::*/
int fb_ConsoleGetTopRow( void )
{
    fb_hSyncGraphicsView( );
    if( view_toprow == -1 ) {
        view_toprow = 0;
    }
	return view_toprow;
}

/*:::::*/
int fb_ConsoleGetBotRow( void )
{
    fb_hSyncGraphicsView( );
    if( view_botrow == -1 ) {
        fb_GetSize( NULL, &view_botrow );
        if( view_botrow!=0 ) {
            --view_botrow;
        } else {
            view_botrow = FB_SCRN_DEFAULT_HEIGHT - 1;
        }
    }

	return view_botrow;
}

/*:::::*/
void fb_ConsoleSetTopBotRows( int top, int bot )
{
    view_toprow = top;
    view_botrow = bot;
    view_rows = -1;
    fb_hSyncGraphicsView( );
}

/*:::::*/
void fb_ConsoleGetView( int *toprow, int *botrow )
{
	*toprow = fb_ConsoleGetTopRow( ) + 1;
    *botrow = fb_ConsoleGetBotRow( ) + 1;
}

/* end of console/io_viewhlp.c */
