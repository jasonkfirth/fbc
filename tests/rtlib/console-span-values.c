/*
 * Project: FreeBASIC runtime library tests
 * File: console-span-values.c
 * Purpose: Check console transfers, TTY controls and signed coordinate limits.
 * Responsibilities: Exercise actual narrow/wide writers through in-memory hooks.
 * This file intentionally does NOT open a console, printer or graphics device.
 */
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "fb.h"
#ifdef __cplusplus
}
#endif

typedef struct CONSOLE_PROBE {
    fb_ConHooks hooks;
    uint64_t hash;
    uint32_t output[2048];
    size_t used;
    size_t writes;
    size_t zero_writes;
    size_t scrolls;
    size_t beeps;
    int wide;
    int fail_write;
} CONSOLE_PROBE;

/* The driver runs one case at a time. The public Beep entry point has no
   hook parameter, so this borrowed pointer only supplies the current fixture. */
static CONSOLE_PROBE *active_probe;
static uint64_t normal_hash = UINT64_C(14695981039346656037);
static unsigned int normal_cases;
static unsigned int edge_cases;

static void check( int condition, const char *message )
{
    if( !condition ) {
        fprintf( stderr, "console-span check failed: %s\n", message );
        exit( 1 );
    }
}

static uint64_t mix( uint64_t hash, uint64_t value )
{
    return (hash ^ value) * UINT64_C(1099511628211);
}

static void note( CONSOLE_PROBE *probe, uint32_t event, int x, int y )
{
    probe->hash = mix( probe->hash, event );
    probe->hash = mix( probe->hash, (uint32_t)x );
    probe->hash = mix( probe->hash, (uint32_t)y );
}

void FBCALL fb_Beep( void )
{
    check( active_probe != NULL, "beep fixture" );
    active_probe->beeps += 1;
    note( active_probe, 3, active_probe->hooks.Coord.X, active_probe->hooks.Coord.Y );
}

static int write_hook( fb_ConHooks *handle, const void *buffer, size_t length )
{
    CONSOLE_PROBE *probe = (CONSOLE_PROBE *)handle->Opaque;
    size_t index;
    int64_t width = (int64_t)handle->Border.Right - handle->Border.Left + 1;

    if( length == 0 ) {
        probe->zero_writes += 1;
        /* The old raw writer makes no progress with a zero-width view.
           Reject that call so baseline qualification cannot spin forever. */
        return width > 0 ? TRUE : FALSE;
    }
    check( width > 0, "positive write width" );
    check( length <= (size_t)width, "write fits a row" );
    check( handle->Coord.X >= handle->Border.Left && handle->Coord.X <= handle->Border.Right,
        "write starts inside a row" );
    check( length <= (size_t)((int64_t)handle->Border.Right - handle->Coord.X + 1),
        "write fits remaining columns" );
    check( probe->used <= 2048 && length <= 2048 - probe->used, "fixture output capacity" );
    probe->writes += 1;
    if( probe->fail_write )
        return FALSE;
    note( probe, 1, handle->Coord.X, handle->Coord.Y );
    probe->hash = mix( probe->hash, length );
    for( index = 0; index < length; ++index ) {
        uint32_t ch = probe->wide ? (uint32_t)((const FB_WCHAR *)buffer)[index] :
            (uint32_t)((const unsigned char *)buffer)[index];
        probe->output[probe->used++] = ch;
        probe->hash = mix( probe->hash, ch );
    }
    return TRUE;
}

static void scroll_hook( fb_ConHooks *handle, int x1, int y1, int x2, int y2, int rows )
{
    CONSOLE_PROBE *probe = (CONSOLE_PROBE *)handle->Opaque;
    int64_t y = (int64_t)handle->Coord.Y - rows;
    check( rows > 0 && y >= INT_MIN && y <= INT_MAX, "representable scroll" );
    check( x1 == handle->Border.Left && y1 == handle->Border.Top &&
        x2 == handle->Border.Right && y2 == handle->Border.Bottom, "scroll viewport" );
    probe->scrolls += 1;
    note( probe, 2, handle->Coord.X, handle->Coord.Y );
    probe->hash = mix( probe->hash, (uint32_t)rows );
    handle->Coord.Y = (int)y;
}

static void init( CONSOLE_PROBE *probe, int wide, int left, int right, int bottom, int x, int y )
{
    memset( probe, 0, sizeof( *probe ) );
    probe->hash = UINT64_C(14695981039346656037);
    probe->wide = wide;
    probe->hooks.Opaque = probe;
    probe->hooks.Scroll = scroll_hook;
    probe->hooks.Write = write_hook;
    probe->hooks.Border.Left = left;
    probe->hooks.Border.Right = right;
    probe->hooks.Border.Top = 0;
    probe->hooks.Border.Bottom = bottom;
    probe->hooks.Coord.X = x;
    probe->hooks.Coord.Y = y;
    active_probe = probe;
}

static void write_text( CONSOLE_PROBE *probe, int tty, const char *text, size_t length, int text_mode )
{
    FB_WCHAR wide[1200];
    size_t index;
    check( length <= 1200, "fixture input capacity" );
    if( probe->wide ) {
        for( index = 0; index < length; ++index )
            wide[index] = (FB_WCHAR)(unsigned char)text[index];
        if( tty )
            fb_ConPrintTTYWstr( &probe->hooks, wide, length, text_mode );
        else
            fb_ConPrintRawWstr( &probe->hooks, wide, length );
    } else if( tty ) {
        fb_ConPrintTTY( &probe->hooks, text, length, text_mode );
    } else {
        fb_ConPrintRaw( &probe->hooks, text, length );
    }
}

static void save_normal( CONSOLE_PROBE *probe )
{
    normal_hash = mix( normal_hash, probe->hash );
    normal_hash = mix( normal_hash, (uint32_t)probe->hooks.Coord.X );
    normal_hash = mix( normal_hash, (uint32_t)probe->hooks.Coord.Y );
    normal_hash = mix( normal_hash, probe->used );
    normal_hash = mix( normal_hash, probe->scrolls );
    normal_hash = mix( normal_hash, probe->beeps );
    normal_cases += 1;
}

static void normal_values( void )
{
    CONSOLE_PROBE probe;
    char long_text[1200];
    char controls[96];
    const char alphabet[] = "ABC\t\n\r\b\a";
    const int widths[] = { 1, 2, 4, 8, 17 };
    uint32_t random = 1;
    int wide, tty, text_mode;
    size_t index, row;

    memset( long_text, 'x', sizeof( long_text ) );
    for( index = 0; index < sizeof( controls ); ++index ) {
        random = random * UINT32_C(1664525) + UINT32_C(1013904223);
        controls[index] = alphabet[(random >> 24) & 7];
    }
    for( wide = 0; wide != 2; ++wide ) {
        init( &probe, wide, 0, 3, 2, 0, 0 );
        write_text( &probe, 0, "ABCDEF", 6, 1 );
        check( probe.used == 6 && probe.hooks.Coord.X == 2 && probe.hooks.Coord.Y == 1,
            "raw wrap values" );
        save_normal( &probe );

        init( &probe, wide, 0, 7, 2, 0, 0 );
        write_text( &probe, 1, "A\tB\nC\rD\bE", 9, 1 );
        check( probe.used == 12 && probe.hooks.Coord.X == 1 && probe.hooks.Coord.Y == 2,
            "TTY tab and movement values" );
        for( index = 0; index < probe.used; ++index )
            check( probe.output[index] == (uint32_t)(unsigned char)"A       BCDE"[index],
                "TTY expanded output" );
        save_normal( &probe );

        for( tty = 0; tty != 2; ++tty ) {
            init( &probe, wide, 0, 0, 2, 0, 0 );
            write_text( &probe, tty, long_text, sizeof( long_text ), 1 );
            check( probe.used == 1200 && probe.scrolls == 1197 &&
                probe.hooks.Coord.X == 0 && probe.hooks.Coord.Y == 3, "buffered scroll values" );
            save_normal( &probe );
            for( row = 0; row < sizeof( widths ) / sizeof( widths[0] ); ++row ) {
                for( text_mode = 0; text_mode != 2; ++text_mode ) {
                    init( &probe, wide, 3, 3 + widths[row] - 1, 2, 3, 0 );
                    write_text( &probe, tty, controls, sizeof( controls ), text_mode );
                    save_normal( &probe );
                    init( &probe, wide, -3, -3 + widths[row] - 1, -1,
                        -3 + widths[row], 4 );
                    write_text( &probe, tty, "ab\tcd\rE\nF\bG\a", 12, text_mode );
                    save_normal( &probe );
                }
            }
            init( &probe, wide, 0, 7, 2, 3, 1 );
            write_text( &probe, tty, "", 0, 1 );
            check( probe.used == 0 && probe.writes == 0 && probe.scrolls == 0,
                "empty input has no callbacks" );
            save_normal( &probe );
        }
        init( &probe, wide, 0, 3, 2, 0, 0 );
        probe.fail_write = 1;
        write_text( &probe, 0, "ABCDEF", 6, 1 );
        check( probe.used == 0 && probe.writes == 1 && probe.hooks.Coord.X == 0 &&
            probe.hooks.Coord.Y == 0, "raw failed hook stops immediately" );
        save_normal( &probe );
    }
    {
        const FB_WCHAR text[] = { 0x03a9, 0x20ac, 0x03bb };
        init( &probe, 1, 0, 1, 2, 0, 0 );
        fb_ConPrintRawWstr( &probe.hooks, text, 3 );
        check( probe.used == 3 && probe.output[0] == 0x03a9 && probe.output[1] == 0x20ac &&
            probe.output[2] == 0x03bb, "wide raw code units" );
        save_normal( &probe );
        init( &probe, 1, 0, 1, 2, 0, 0 );
        fb_ConPrintTTYWstr( &probe.hooks, text, 3, 1 );
        check( probe.used == 3 && probe.output[2] == 0x03bb, "wide TTY code units" );
        save_normal( &probe );
    }
}

static void edge_values( int after )
{
    CONSOLE_PROBE probe;
    unsigned int baseline_zero_writes = 0;
    int wide, tty;
    for( wide = 0; wide != 2; ++wide ) {
        init( &probe, wide, 0, -1, 2, 0, 0 );
        write_text( &probe, 0, "A", 1, 1 );
        check( probe.used == 0 && probe.scrolls == 0, "zero-width raw emits no data" );
        check( probe.zero_writes == (after ? 0u : 1u), "zero-width progress regression" );
        baseline_zero_writes += (unsigned int)probe.zero_writes;
        edge_cases += 1;
        if( !after )
            continue;
        for( tty = 0; tty != 2; ++tty ) {
            const int left[] = { 0, 7, INT_MIN, 0, 0 };
            const int right[] = { -1, 2, INT_MAX, 3, 3 };
            const int x[] = { 0, 7, 0, -1, 5 };
            size_t row;
            for( row = 0; row < sizeof( left ) / sizeof( left[0] ); ++row ) {
                init( &probe, wide, left[row], right[row], 2, x[row], 0 );
                write_text( &probe, tty, "A", 1, 1 );
                check( probe.used == 0 && probe.writes == 0 && probe.zero_writes == 0 &&
                    probe.scrolls == 0, "invalid geometry is a no-op" );
                edge_cases += 1;
            }
            init( &probe, wide, INT_MAX - 1, INT_MAX, -1, INT_MAX - 1, 0 );
            write_text( &probe, tty, "ABC", 3, 1 );
            check( probe.used == 3 && probe.hooks.Coord.X == INT_MAX && probe.hooks.Coord.Y == 1,
                "positive coordinate endpoint" );
            edge_cases += 1;
            init( &probe, wide, INT_MIN, INT_MIN + 1, -1, INT_MIN, 0 );
            write_text( &probe, tty, "ABC", 3, 1 );
            check( probe.used == 3 && probe.hooks.Coord.X == INT_MIN + 1 && probe.hooks.Coord.Y == 1,
                "negative coordinate endpoint" );
            edge_cases += 1;
            init( &probe, wide, 0, 0, 2, 0, INT_MAX - 1 );
            write_text( &probe, tty, "AB", 2, 1 );
            check( probe.used == 2 && probe.hooks.Coord.X == 0 && probe.hooks.Coord.Y == 3,
                "pending output flushes before virtual row overflow" );
            edge_cases += 1;
            init( &probe, wide, 0, 0, 2, 0, INT_MAX );
            write_text( &probe, tty, "A", 1, 1 );
            check( probe.used == 1 && probe.hooks.Coord.X == 0 && probe.hooks.Coord.Y == 3,
                "bounded endpoint row can scroll" );
            edge_cases += 1;
            init( &probe, wide, 0, 0, -1, 0, INT_MAX );
            write_text( &probe, tty, "A", 1, 1 );
            check( probe.used == 0 && probe.scrolls == 0, "unbounded row overflow stops" );
            edge_cases += 1;
        }
        init( &probe, wide, INT_MAX - 1, INT_MAX, -1, INT_MAX, 0 );
        write_text( &probe, 1, "\t", 1, 1 );
        check( probe.used == 7 && probe.hooks.Coord.X == INT_MAX - 1 && probe.hooks.Coord.Y == 4,
            "tab at coordinate endpoint" );
        edge_cases += 1;
    }
    if( after ) {
        fb_ConPrintRaw( NULL, NULL, 0 );
        fb_ConPrintRawWstr( NULL, NULL, 0 );
        fb_ConPrintTTY( NULL, NULL, 0, 1 );
        fb_ConPrintTTYWstr( NULL, NULL, 0, 1 );
        edge_cases += 4;
    }
    printf( "EDGE cases=%u zero_progress_calls=%u\n", edge_cases, baseline_zero_writes );
}

int main( int argc, char **argv )
{
    int after;
    check( argc == 2, "before/after phase argument" );
    after = strcmp( argv[1], "after" ) == 0;
    check( after || strcmp( argv[1], "before" ) == 0, "recognized phase" );
    normal_values();
    printf( "NORMAL cases=%u hash=%016llx\n", normal_cases, (unsigned long long)normal_hash );
    edge_values( after );
    puts( "CONSOLE_SPAN_VALUES_PASS" );
    return 0;
}

/* end of console-span-values.c */
