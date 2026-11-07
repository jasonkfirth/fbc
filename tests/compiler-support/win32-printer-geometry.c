/*
    Project: FreeBASIC Win32 printer regression tests
    File: win32-printer-geometry.c
    Purpose: Check printer counts, font metrics and TTY cell geometry.
    Responsibilities: Intercept device calls and test ordinary character grids.
    This file intentionally does NOT enumerate real printers or send print jobs.
*/

#include "../../src/rtlib/fb.h"
#include <windows.h>
#include <winspool.h>
#include <stddef.h>

static unsigned checks, failures, row_mismatches;
static DWORD enum_count, enum_error;
static unsigned enum_calls, tty_calls;
static int expected_right, expected_bottom, expected_x, expected_y, expected_wide;
static int metrics_available = TRUE;
static TEXTMETRIC supplied_metrics;
static char printer_name[] = "Geometry Test";
static char port_name[] = "LPT1:";

static int test_check( int condition, const char *expression, int line )
{
    ++checks;
    if( !condition ) {
        ++failures;
        fprintf( stderr, "printer geometry line %d: %s\n", line, expression );
    }
    return condition;
}

#define CHECK(expression) test_check( !!(expression), #expression, __LINE__ )

static DWORD WINAPI test_GetLastError( void );
static BOOL WINAPI test_EnumPrinters( DWORD, LPTSTR, DWORD, LPBYTE, DWORD, LPDWORD, LPDWORD );
static HGDIOBJ WINAPI test_SelectObject( HDC, HGDIOBJ );
static BOOL WINAPI test_GetTextMetrics( HDC, LPTEXTMETRIC );
static void test_PrintTTY( fb_ConHooks *, const char *, size_t, int );
static void test_PrintTTYWstr( fb_ConHooks *, const FB_WCHAR *, size_t, int );
static int test_CheckScroll( fb_ConHooks * );

/* The SDK declarations are loaded before these source-only substitutions. */
#undef EnumPrinters
#define EnumPrinters test_EnumPrinters
#define GetLastError test_GetLastError
#define SelectObject test_SelectObject
#undef GetTextMetrics
#define GetTextMetrics test_GetTextMetrics
#define fb_ConPrintTTY test_PrintTTY
#define fb_ConPrintTTYWstr test_PrintTTYWstr
#define fb_hConCheckScroll test_CheckScroll

#ifndef PRINTER_IMPLEMENTATION
#define PRINTER_IMPLEMENTATION "../../src/rtlib/drivers/win32/io_printer.c"
#endif
#include PRINTER_IMPLEMENTATION

static DWORD WINAPI test_GetLastError( void )
{
    return enum_error;
}

static BOOL WINAPI test_EnumPrinters( DWORD flags, LPTSTR name, DWORD level,
    LPBYTE buffer, DWORD size, LPDWORD needed, LPDWORD returned )
{
    DWORD i;
    size_t entry_size = (level == 2) ? sizeof(PRINTER_INFO_2) : sizeof(PRINTER_INFO_5);
    DWORD required = (DWORD)(entry_size * enum_count);
    ++enum_calls;
    if( !CHECK( name == NULL && (level == 2 || level == 5) ) ||
        !CHECK( flags == (level == 2 ? PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS : PRINTER_ENUM_DEFAULT) ) ) {
        enum_error = ERROR_INVALID_PARAMETER;
        return FALSE;
    }
    *needed = required;
    *returned = 0;
    if( enum_count == 0 )
        return TRUE;
    if( buffer == NULL || size < required ) {
        enum_error = ERROR_INSUFFICIENT_BUFFER;
        return FALSE;
    }
    memset( buffer, 0, required );
    for( i = 0; i < enum_count; ++i ) {
        if( level == 2 ) {
            PRINTER_INFO_2 *info = (PRINTER_INFO_2 *)buffer + i;
            info->pPrinterName = printer_name;
            info->pPortName = port_name;
        } else {
            PRINTER_INFO_5 *info = (PRINTER_INFO_5 *)buffer + i;
            info->pPrinterName = printer_name;
            info->pPortName = port_name;
        }
    }
    *returned = enum_count;
    return TRUE;
}

static HGDIOBJ WINAPI test_SelectObject( HDC dc, HGDIOBJ object )
{
    /* These identifiers are only compared by interceptors, never sent to GDI. */
    CHECK( dc == (HDC)(uintptr_t)1 );
    CHECK( object == (HGDIOBJ)(uintptr_t)2 );
    return (HGDIOBJ)(uintptr_t)3;
}

static BOOL WINAPI test_GetTextMetrics( HDC dc, LPTEXTMETRIC metrics )
{
    CHECK( dc == (HDC)(uintptr_t)1 );
    if( !metrics_available )
        return FALSE;
    *metrics = supplied_metrics;
    return TRUE;
}

static void check_hook( fb_ConHooks *hook, size_t length, int text_mode, int wide )
{
    ++tty_calls;
    CHECK( length == 2 && text_mode == TRUE );
    CHECK( wide == expected_wide );
    CHECK( hook->Border.Left == 0 && hook->Border.Top == 0 );
    CHECK( hook->Border.Right == expected_right );
    if( hook->Border.Bottom != expected_bottom ) {
        ++row_mismatches;
        CHECK( hook->Border.Bottom == expected_bottom );
    } else {
        CHECK( hook->Border.Bottom == expected_bottom );
    }
    CHECK( hook->Coord.X == expected_x && hook->Coord.Y == expected_y );
    CHECK( hook->Write == (wide ? fb_hHookConPrinterWriteWstr : fb_hHookConPrinterWrite) );
    hook->Coord.X += (int)length;
}

static void test_PrintTTY( fb_ConHooks *hook, const char *text, size_t length, int text_mode )
{
    CHECK( text[0] == 'A' && text[1] == 'B' );
    check_hook( hook, length, text_mode, FALSE );
}

static void test_PrintTTYWstr( fb_ConHooks *hook, const FB_WCHAR *text, size_t length, int text_mode )
{
    CHECK( text[0] == 'A' && text[1] == 'B' );
    check_hook( hook, length, text_mode, TRUE );
}

static int test_CheckScroll( fb_ConHooks *hook )
{
    CHECK( hook->Coord.X == expected_x + 2 && hook->Coord.Y == expected_y );
    return FALSE;
}

static void check_counts( void )
{
    DWORD requested;
    for( requested = 0; requested < 4; ++requested ) {
#ifdef PRINTER_BASELINE
        int count;
#else
        DWORD count;
#endif
        PRINTER_INFO_2 *all;
        PRINTER_INFO_5 *defaults;
        enum_count = requested;
        enum_calls = 0;
        all = GetPrinters( &count );
        CHECK( (DWORD)count == requested );
        CHECK( enum_calls == (requested ? 2u : 1u) );
        CHECK( (all != NULL) == (requested != 0) );
        if( all != NULL ) CHECK( all[requested - 1].pPrinterName == printer_name );
        free( all );
        enum_calls = 0;
        defaults = GetDefaultPrinters( &count );
        CHECK( (DWORD)count == requested );
        CHECK( enum_calls == (requested ? 2u : 1u) );
        CHECK( (defaults != NULL) == (requested != 0) );
        if( defaults != NULL ) CHECK( defaults[requested - 1].pPrinterName == printer_name );
        free( defaults );
    }
}

static void check_fonts( void )
{
    W32_PRINTER_INFO info = { 0 };
    LOGFONT font;
    info.hDc = (HDC)(uintptr_t)1;
    info.Emu.hFont = (HFONT)(uintptr_t)2;
    info.Emu.dwDPI_X = info.Emu.dwDPI_Y = 96;
    EmuBuild_LOGFONT( &font, &info, 12 );
    CHECK( font.lfHeight == 13 && font.lfWidth == 8 );
    EmuBuild_LOGFONT( &font, &info, 0 );
    CHECK( font.lfHeight == 13 && font.lfWidth == 0 );
    supplied_metrics.tmMaxCharWidth = 8;
    supplied_metrics.tmHeight = 16;
    EmuUpdateInfo( &info );
    CHECK( info.Emu.dwFontSizeX == 8 && info.Emu.dwFontSizeY == 16 );
    supplied_metrics.tmMaxCharWidth = supplied_metrics.tmHeight = 0;
    EmuUpdateInfo( &info );
    CHECK( info.Emu.dwFontSizeX == 1 && info.Emu.dwFontSizeY == 1 );
#ifndef PRINTER_BASELINE
    metrics_available = FALSE;
    EmuUpdateInfo( &info );
    CHECK( info.Emu.dwFontSizeX == 1 && info.Emu.dwFontSizeY == 1 );
    metrics_available = TRUE;
#endif
}

int main( void )
{
    static const int cases[][6] = {
        { 8, 16, 80, 32, 9, 1 },
        { 8, 16, 80, 40, 9, 1 },
        { 12, 20, 120, 72, 9, 2 },
        { 9, 9, 90, 27, 9, 2 },
        { 6, 12, 60, 60, 9, 4 }
    };
    size_t i;
    int wide;
    FB_WCHAR text[] = { 'A', 'B' };
    check_counts();
    check_fonts();
    printf( "PRINTER_LAYOUT %zu %zu %zu\n", sizeof(W32_PRINTER_INFO),
        offsetof(W32_PRINTER_INFO, Emu.dwCurrentX), offsetof(W32_PRINTER_INFO, Emu.dwFontSizeY) );
    for( i = 0; i < sizeof(cases)/sizeof(cases[0]); ++i ) {
        for( wide = 0; wide < 2; ++wide ) {
            W32_PRINTER_INFO info = { 0 };
            info.Emu.dwFontSizeX = cases[i][0];
            info.Emu.dwFontSizeY = cases[i][1];
            info.Emu.dwSizeX = cases[i][2];
            info.Emu.dwSizeY = cases[i][3];
            info.Emu.dwCurrentX = 2 * cases[i][0];
            info.Emu.dwCurrentY = cases[i][1];
            expected_x = 2; expected_y = 1;
            expected_right = cases[i][4]; expected_bottom = cases[i][5];
            expected_wide = wide;
            EmuPrint_TTY( &info, wide ? (const void *)text : (const void *)"AB", 2, wide );
            CHECK( info.Emu.dwCurrentX == 4 * cases[i][0] && info.Emu.dwCurrentY == cases[i][1] );
        }
    }
    CHECK( tty_calls == 10 );
    printf( "PRINTER_GEOMETRY checks=%u failures=%u vertical=%u\n", checks, failures, row_mismatches );
    if( failures != row_mismatches ) return 2;
    if( row_mismatches ) return 3;
    puts( "PRINTER_GEOMETRY_PASS" );
    return 0;
}

/* end of win32-printer-geometry.c */
