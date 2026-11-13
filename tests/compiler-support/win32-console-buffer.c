/*
    Project: FreeBASIC Win32 console regression tests
    File: win32-console-buffer.c
    Purpose: Check console cell buffers, scrolling and redirected text writes.
    Responsibilities: Intercept native output and compare ordinary narrow/wide results.
    This file intentionally does NOT modify the user's console or create large files.
*/

#include "../../src/rtlib/fb.h"
#include <windows.h>
#include <limits.h>

static unsigned checks, failures, output_calls, fill_calls, scroll_calls;
static unsigned allocations, releases, live_allocations, byte_calls;
static int fail_allocation, fail_output, zero_progress, fail_bytes;
static WORD expected_attributes = 7;
static size_t expected_length;
static int expected_x, expected_y;
static DWORD first_byte_request;
static unsigned char captured[128];
static size_t captured_length;
static CHAR_INFO expected_cells[SHRT_MAX];
static int lock_depth;
#ifdef ENABLE_MT
static void test_lock( void ) { ++lock_depth; }
static void test_unlock( void ) { --lock_depth; }
#endif

static int check( int condition, const char *expression, int line )
{
    ++checks;
    if( !condition ) {
        ++failures;
        fprintf( stderr, "console buffer line %d: %s\n", line, expression );
    }
    return condition;
}
#define CHECK(expression) check( !!(expression), #expression, __LINE__ )

static void *test_malloc( size_t );
static void test_free( void * );
static BOOL WINAPI test_WriteFile( HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED );
static BOOL WINAPI test_WriteConsoleOutput( HANDLE, const CHAR_INFO *, COORD, COORD, PSMALL_RECT );
static BOOL WINAPI test_Scroll( HANDLE, const SMALL_RECT *, const SMALL_RECT *, COORD, const CHAR_INFO * );
static BOOL WINAPI test_FillAttribute( HANDLE, WORD, DWORD, COORD, LPDWORD );
#ifdef CONSOLE_BUFFER_WIDE
static BOOL WINAPI test_FillWide( HANDLE, WCHAR, DWORD, COORD, LPDWORD );
#else
static BOOL WINAPI test_FillChar( HANDLE, CHAR, DWORD, COORD, LPDWORD );
#endif
static BOOL WINAPI test_GetScreen( HANDLE, PCONSOLE_SCREEN_BUFFER_INFO );
static BOOL WINAPI test_SetCursor( HANDLE, COORD );
static BOOL WINAPI test_SetWindow( HANDLE, BOOL, const SMALL_RECT * );
static HANDLE test_GetHandle( int );
static void test_GetView( int *, int * );
static void test_GetWindow( int *, int *, int *, int * );
static void FBCALL test_UpdateWindow( void );
#ifdef CONSOLE_BUFFER_WIDE
static void test_TTYWide( fb_ConHooks *, const FB_WCHAR *, size_t, int );
#else
static void test_TTY( fb_ConHooks *, const char *, size_t, int );
#endif

#define malloc test_malloc
#define free test_free
#define WriteFile test_WriteFile
#undef WriteConsoleOutput
#define WriteConsoleOutput test_WriteConsoleOutput
#define WriteConsoleOutputW test_WriteConsoleOutput
#undef ScrollConsoleScreenBuffer
#define ScrollConsoleScreenBuffer test_Scroll
#define ScrollConsoleScreenBufferW test_Scroll
#define FillConsoleOutputAttribute test_FillAttribute
#undef FillConsoleOutputCharacter
#define FillConsoleOutputCharacter test_FillChar
#define FillConsoleOutputCharacterW test_FillWide
#define GetConsoleScreenBufferInfo test_GetScreen
#define SetConsoleCursorPosition test_SetCursor
#define SetConsoleWindowInfo test_SetWindow
#define fb_hConsoleGetHandle test_GetHandle
#define fb_ConsoleGetView test_GetView
#define fb_hConsoleGetWindow test_GetWindow
#define fb_hUpdateConsoleWindow test_UpdateWindow
#define fb_ConPrintTTY test_TTY
#define fb_ConPrintTTYWstr test_TTYWide
#define __fb_con test_console
#define fb_console_context test_console
/* Keep the test's console state and lock bookkeeping independent of the
   normal runtime's process-wide startup and real console handles. A native
   C identifier can carry an explicit ABI label, which must not bind this
   fixture's mock state to the runtime's real context. */
#pragma push_macro("FB_C_SYMBOL")
#undef FB_C_SYMBOL
#define FB_C_SYMBOL(name)
#ifdef ENABLE_MT
#undef FB_LOCK
#undef FB_UNLOCK
#define FB_LOCK() test_lock()
#define FB_UNLOCK() test_unlock()
#endif

#ifndef CONSOLE_BUFFER_IMPLEMENTATION
#ifdef CONSOLE_BUFFER_WIDE
#define CONSOLE_BUFFER_IMPLEMENTATION "../../src/rtlib/drivers/win32/io_printbuff_wstr.c"
#else
#define CONSOLE_BUFFER_IMPLEMENTATION "../../src/rtlib/drivers/win32/io_printbuff.c"
#endif
#endif
#include CONSOLE_BUFFER_IMPLEMENTATION
#pragma pop_macro("FB_C_SYMBOL")

#undef malloc
#undef free

FB_CONSOLE_CTX __fb_con;

static void *test_malloc( size_t size )
{
    void *result;
    ++allocations;
    if( fail_allocation )
        return NULL;
    result = malloc( size );
    if( result != NULL )
        ++live_allocations;
    return result;
}

static void test_free( void *ptr )
{
    if( ptr != NULL ) {
        CHECK( live_allocations != 0 );
        --live_allocations;
        ++releases;
    }
    free( ptr );
}

static BOOL WINAPI test_WriteConsoleOutput( HANDLE output, const CHAR_INFO *cells,
    COORD size, COORD origin, PSMALL_RECT region )
{
    size_t i;
    ++output_calls;
    CHECK( output == (HANDLE)(uintptr_t)1 );
    CHECK( (size_t)size.X == expected_length && size.Y == 1 );
    CHECK( origin.X == 0 && origin.Y == 0 );
    CHECK( region->Left == expected_x && region->Top == expected_y );
    CHECK( region->Right == expected_x + (int)expected_length - 1 && region->Bottom == expected_y );
    for( i = 0; i < expected_length; ++i ) {
        if( cells[i].Attributes != expected_attributes ||
#ifdef CONSOLE_BUFFER_WIDE
            cells[i].Char.UnicodeChar != expected_cells[i].Char.UnicodeChar ) {
#else
            cells[i].Char.AsciiChar != expected_cells[i].Char.AsciiChar ) {
#endif
            CHECK( FALSE );
            break;
        }
    }
    return !fail_output;
}

static BOOL WINAPI test_WriteFile( HANDLE output, LPCVOID data, DWORD length,
    LPDWORD written, LPOVERLAPPED overlapped )
{
    DWORD amount = length < 2 ? length : 2;
    ++byte_calls;
    if( byte_calls == 1 )
        first_byte_request = length;
    CHECK( output == (HANDLE)(uintptr_t)1 && overlapped == NULL );
    *written = 0;
    if( fail_bytes )
        return FALSE;
    if( zero_progress )
        return TRUE;
    CHECK( captured_length + amount <= sizeof(captured) );
    if( captured_length + amount <= sizeof(captured) ) {
        memcpy( captured + captured_length, data, amount );
        captured_length += amount;
        *written = amount;
    }
    return TRUE;
}

static BOOL WINAPI test_Scroll( HANDLE output, const SMALL_RECT *source,
    const SMALL_RECT *clip, COORD destination, const CHAR_INFO *fill )
{
    ++scroll_calls;
    CHECK( output == (HANDLE)(uintptr_t)1 && clip == NULL );
    CHECK( source->Left == destination.X && source->Top > destination.Y );
    CHECK( source->Right >= source->Left && source->Bottom >= source->Top );
    CHECK( fill->Attributes == expected_attributes );
    return TRUE;
}

static BOOL WINAPI test_FillAttribute( HANDLE output, WORD attribute, DWORD width,
    COORD position, LPDWORD written )
{
    ++fill_calls;
    CHECK( output == (HANDLE)(uintptr_t)1 && attribute == expected_attributes );
    CHECK( width == 10 && position.X == 3 && position.Y >= 3 && position.Y <= 7 );
    *written = width;
    return TRUE;
}

#ifndef CONSOLE_BUFFER_WIDE
static BOOL WINAPI test_FillChar( HANDLE output, CHAR value, DWORD width,
    COORD position, LPDWORD written )
{
    CHECK( value == ' ' );
    return test_FillAttribute( output, expected_attributes, width, position, written );
}
#else
static BOOL WINAPI test_FillWide( HANDLE output, WCHAR value, DWORD width,
    COORD position, LPDWORD written )
{
    CHECK( value == L' ' );
    return test_FillAttribute( output, expected_attributes, width, position, written );
}
#endif

static BOOL WINAPI test_GetScreen( HANDLE output, PCONSOLE_SCREEN_BUFFER_INFO info )
{
    CHECK( output == (HANDLE)(uintptr_t)1 );
    memset( info, 0, sizeof(*info) );
    info->dwSize.X = 80; info->dwSize.Y = 25;
    info->wAttributes = expected_attributes;
    return TRUE;
}

static BOOL WINAPI test_SetCursor( HANDLE output, COORD position )
{
    CHECK( output == (HANDLE)(uintptr_t)1 && position.X >= 0 && position.Y >= 0 );
    return TRUE;
}

static BOOL WINAPI test_SetWindow( HANDLE output, BOOL absolute, const SMALL_RECT *rect )
{
    CHECK( output == (HANDLE)(uintptr_t)1 && absolute && rect->Bottom >= rect->Top );
    return TRUE;
}

static HANDLE test_GetHandle( int is_input )
{
    CHECK( !is_input );
    return (HANDLE)(uintptr_t)1;
}

static void test_GetView( int *top, int *bottom ) { *top = 1; *bottom = 25; }
static void test_GetWindow( int *left, int *top, int *cols, int *rows )
{ *left = 0; *top = 0; *cols = 80; *rows = 25; }
static void FBCALL test_UpdateWindow( void ) { }

#ifndef CONSOLE_BUFFER_WIDE
static void test_TTY( fb_ConHooks *hook, const char *text, size_t length, int text_mode )
{ CHECK( text_mode ); CHECK( hook->Write( hook, text, length ) == TRUE ); }
#else
static void test_TTYWide( fb_ConHooks *hook, const FB_WCHAR *text, size_t length, int text_mode )
{ CHECK( text_mode ); CHECK( hook->Write( hook, text, length ) == TRUE ); }
#endif

static void test_cells( void )
{
#ifdef CONSOLE_BUFFER_WIDE
    static FB_WCHAR wide[SHRT_MAX];
    const void *text = wide;
#else
    static char narrow[SHRT_MAX];
    const void *text = narrow;
#endif
    const size_t lengths[] = { 1, 17, 256, 257, SHRT_MAX };
    fb_PrintInfo info = { 0 };
    fb_ConHooks hook = { 0 };
    size_t i, n;
    info.hOutput = (HANDLE)(uintptr_t)1;
    info.BufferSize.X = SHRT_MAX; info.BufferSize.Y = 25;
    info.wAttributes = expected_attributes;
    hook.Opaque = &info; hook.Coord.X = 0; hook.Coord.Y = 4;
    for( i = 0; i < SHRT_MAX; ++i ) {
        expected_cells[i].Attributes = expected_attributes;
#ifdef CONSOLE_BUFFER_WIDE
        wide[i] = (FB_WCHAR)(0x100 + i % 26);
        expected_cells[i].Char.UnicodeChar = wide[i];
#else
        narrow[i] = (char)('A' + i % 26);
        expected_cells[i].Char.UnicodeChar = (unsigned char)narrow[i];
#endif
    }
    for( n = 0; n < sizeof(lengths)/sizeof(lengths[0]); ++n ) {
        unsigned before = output_calls;
        expected_length = lengths[n]; expected_x = 0; expected_y = 4;
        CHECK( fb_hHookConWrite( &hook, text, expected_length ) == TRUE );
        CHECK( output_calls == before + 1 && live_allocations == 0 );
    }
#ifndef CONSOLE_BUFFER_BASELINE
    {
        unsigned before = output_calls, released = releases;
        fail_allocation = TRUE;
        CHECK( fb_hHookConWrite( &hook, text, 257 ) == FALSE );
        CHECK( output_calls == before && live_allocations == 0 );
        fail_allocation = FALSE;
        fail_output = TRUE; expected_length = 257;
        CHECK( fb_hHookConWrite( &hook, text, 257 ) == FALSE );
        CHECK( releases == released + 1 && live_allocations == 0 );
        fail_output = FALSE;
        before = output_calls;
        CHECK( fb_hHookConWrite( &hook, text, 0 ) == TRUE );
        hook.Coord.X = -1;
        CHECK( fb_hHookConWrite( &hook, text, 1 ) == FALSE );
        hook.Coord.X = SHRT_MAX - 1;
        CHECK( fb_hHookConWrite( &hook, text, 2 ) == FALSE );
        CHECK( output_calls == before && live_allocations == 0 );
    }
#endif
}

static void test_scroll( void )
{
    fb_PrintInfo info = { 0 };
    fb_ConHooks hook = { 0 };
    info.hOutput = (HANDLE)(uintptr_t)1;
    info.BufferSize.X = 80; info.BufferSize.Y = 25;
    info.wAttributes = expected_attributes; info.fViewSet = TRUE;
    hook.Opaque = &info;
    hook.Border.Left = 3; hook.Border.Right = 12;
    hook.Border.Top = 3; hook.Border.Bottom = 7; hook.Coord.Y = 8;
    fb_hHookConScroll( &hook, 3, 3, 12, 7, 1 );
    CHECK( scroll_calls == 1 && fill_calls == 0 && hook.Coord.Y == 7 );
    fb_hHookConScroll( &hook, 3, 3, 12, 7, 5 );
    CHECK( scroll_calls == 1 && fill_calls == 10 && hook.Coord.Y == 7 );
    info.fViewSet = FALSE; info.BufferSize.Y = 40;
    hook.Border.Left = 0; hook.Border.Right = 79;
    hook.Border.Top = 0; hook.Border.Bottom = 24; hook.Coord.Y = 25;
    fb_hHookConScroll( &hook, 0, 0, 79, 24, 1 );
    CHECK( hook.Border.Top == 1 && hook.Border.Bottom == 25 && hook.Coord.Y == 25 );
    CHECK( scroll_calls == 1 && fill_calls == 10 );
}

static void test_screen( void )
{
    unsigned before = output_calls;
    __fb_con.window.Left = __fb_con.window.Top = 0;
    __fb_con.window.Right = 79; __fb_con.window.Bottom = 24;
    expected_length = 3; expected_x = 0; expected_y = 0;
#ifdef CONSOLE_BUFFER_WIDE
    {
        const FB_WCHAR text[] = { 0x100, 0x101, 0x102, 0 };
        fb_ConsolePrintBufferWstr( text, 0 );
    }
#else
    fb_ConsolePrintBuffer( "ABC", 0 );
#endif
    CHECK( output_calls == before + 1 && live_allocations == 0 );
}

static void test_redirect( void )
{
    __fb_con.window.Left = __fb_con.window.Right = 0;
    byte_calls = 0; captured_length = 0;
#ifdef CONSOLE_BUFFER_WIDE
    {
        static const FB_WCHAR text[] = { 'A', 0x20ac, 'Z', 0 };
        static const unsigned char expected[] = { 'A', 0xe2, 0x82, 0xac, 'Z' };
        fb_ConsolePrintBufferWstr( text, 0 );
        CHECK( captured_length == sizeof(expected) && memcmp(captured, expected, sizeof(expected)) == 0 );
    }
#else
    fb_ConsolePrintBuffer( "ABCDE", 0 );
    CHECK( captured_length == 5 && memcmp(captured, "ABCDE", 5) == 0 );
#endif
    CHECK( byte_calls == 3 && live_allocations == 0 );
#ifndef CONSOLE_BUFFER_BASELINE
    byte_calls = 0; zero_progress = TRUE;
#ifdef CONSOLE_BUFFER_WIDE
    fb_ConsolePrintBufferWstr( L"a", 0 );
#else
    fb_ConsolePrintBuffer( "a", 0 );
#endif
    CHECK( byte_calls == 1 && live_allocations == 0 );
    zero_progress = FALSE; fail_bytes = TRUE; byte_calls = 0;
#ifdef CONSOLE_BUFFER_WIDE
    fb_ConsolePrintBufferWstr( L"a", 0 );
#else
    fb_ConsolePrintBuffer( "a", 0 );
#ifdef _WIN64
    /* The interceptor rejects the first request without reading the tiny
       input. Check the API count boundary without allocating a large string. */
    byte_calls = 0;
    fb_ConsolePrintBufferEx( "a", (size_t)MAXDWORD + 1, 0 );
    CHECK( first_byte_request == MAXDWORD );
#endif
#endif
    CHECK( byte_calls == 1 && live_allocations == 0 );
    fail_bytes = FALSE;
#endif
}

int main( void )
{
    test_cells();
    test_scroll();
    test_screen();
    test_redirect();
    CHECK( lock_depth == 0 );
    printf( "CONSOLE_BUFFER checks=%u failures=%u allocations=%u releases=%u\n",
        checks, failures, allocations, releases );
    if( failures != 0 )
        return 1;
    puts( "CONSOLE_BUFFER_PASS" );
    return 0;
}

/* end of win32-console-buffer.c */
