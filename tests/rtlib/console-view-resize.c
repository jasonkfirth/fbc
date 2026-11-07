/*
 * Project: FreeBASIC runtime tests
 * File: rtlib/console-view-resize.c
 * Purpose: Regress stale VIEW PRINT bounds after graphics resizing.
 * Responsibilities: Check production row helpers with changing size hooks.
 * This file intentionally does NOT simulate rendering or a native compositor.
 *
 * cc -std=gnu11 -Wall -Wextra -Werror -I src/rtlib \
 *    tests/rtlib/console-view-resize.c \
 *    -o /tmp/console-view-resize && /tmp/console-view-resize
 */
#ifndef FB_CONSOLE_VIEW_SOURCE
#define FB_CONSOLE_VIEW_SOURCE "../../src/rtlib/console/io_viewhlp.c"
#endif
#include FB_CONSOLE_VIEW_SOURCE
#include <limits.h>

FB_RTLIB_CTX fb_ctx;
static int rows = 80, console_queries, checks;

static void graphics_size( int *columns, int *height )
{
    if( columns != NULL )
        *columns = 45;
    if( height != NULL )
        *height = rows;
}

FBCALL void fb_GetSize( int *columns, int *height )
{
    /* The normal console path must keep its one lazy size query. Graphics
       synchronization must use the selected hook without entering this lock. */
    if( fb_ctx.hooks.getsizeproc != NULL ) {
        fprintf(stderr, "CONSOLE_VIEW_RESIZE_FAIL runtime hook reentered\n");
        exit(1);
    }
    ++console_queries;
    graphics_size(columns, height);
}

static void check( int condition )
{
    ++checks;
    if( !condition ) {
        fprintf(stderr, "CONSOLE_VIEW_RESIZE_FAIL check=%d\n", checks);
        exit(1);
    }
}

static void range( int top, int bottom )
{
    int actual_top, actual_bottom;
    check(fb_ConsoleGetTopRow() == top);
    check(fb_ConsoleGetBotRow() == bottom);
    fb_ConsoleGetView(&actual_top, &actual_bottom);
    check(actual_top == top + 1 && actual_bottom == bottom + 1);
}

int main( void )
{
    fb_ConsoleSetTopBotRows(-1, -1);
    range(0, 79);
    rows = 65;
    range(0, 79);
    check(console_queries == 1);

    fb_ctx.hooks.getsizeproc = graphics_size;
    rows = 80;
    fb_ConsoleSetTopBotRows(0, 79);
    range(0, 79);
    rows = 65;
    range(0, 64);
    rows = 100;
    range(0, 99);

    fb_ConsoleSetTopBotRows(4, 20);
    rows = 80;
    range(4, 20);
    rows = 10;
    range(4, 9);
    rows = 2;
    range(1, 1);
    rows = 1;
    range(0, 0);
    rows = 65;
    fb_ConsoleSetTopBotRows(-1, -1);
    range(0, 64);
    fb_ConsoleSetTopBotRows(INT_MAX, INT_MAX);
    range(64, 64);

    /* A transient unavailable hook size must not create a negative range. */
    rows = 0;
    range(64, 64);
    rows = -1;
    range(64, 64);
    rows = 80;
    range(64, 79);
    check(console_queries == 1);
    printf("CONSOLE_VIEW_RESIZE_PASS checks=%d\n", checks);
    return 0;
}

/* end of rtlib/console-view-resize.c */
