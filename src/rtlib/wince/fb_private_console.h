/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/fb_private_console.h

    Purpose:

        Define the private logical-console model used on Windows CE.

    Responsibilities:

        - describe the bounded text pages maintained by the runtime
        - expose helpers shared by the Windows CE console units
        - preserve handle-shaped compatibility entry points for generic code

    This file intentionally does NOT contain:

        - desktop console screen-buffer declarations
        - graphics-window state
        - keyboard queue storage
*/

#ifndef FB_RT_WINCE_PRIVATE_CONSOLE_H
#define FB_RT_WINCE_PRIVATE_CONSOLE_H

#include <windows.h>

typedef struct FB_WINCE_CONSOLE_CELL {
	FB_WCHAR character;
	unsigned char attribute;
} FB_WINCE_CONSOLE_CELL;

typedef struct FB_WINCE_CONSOLE_PAGE {
	FB_WINCE_CONSOLE_CELL *cells;
} FB_WINCE_CONSOLE_PAGE;

typedef struct FB_CONSOLE_CTX {
	FB_WINCE_CONSOLE_PAGE pages[FB_CONSOLE_MAXPAGES];
	int active;
	int visible;
	int cursor_x;
	int cursor_y;
	int columns;
	int rows;
	int cursor_visible;
	unsigned int foreground;
	unsigned int background;
} FB_CONSOLE_CTX;

extern FB_CONSOLE_CTX __fb_con;

int fb_hWinCEConsoleInitialize( void );
void fb_hWinCEConsoleShutdown( void );
int fb_hWinCEConsoleEnsurePage( int page );
int fb_hWinCEConsoleResize( int columns, int rows );
FB_WINCE_CONSOLE_CELL *fb_hWinCEConsoleCell( int page, int column, int row );
void fb_hWinCEConsoleClearRegion( int page, int left, int top, int right,
	                              int bottom );
void fb_hWinCEConsoleScrollRegion( int page, int left, int top, int right,
	                               int bottom, int rows );
void fb_hWinCEConsoleWrite( const char *text, size_t length );

int fb_hConsoleTranslateKey( char ascii_character, WORD scan_code,
	                         WORD virtual_key, DWORD control_state,
	                         int enhanced_only );
int fb_hVirtualToScancode( int virtual_key );
void fb_InitConsoleWindow( void );
FBCALL void fb_hRestoreConsoleWindow( void );
FBCALL void fb_hUpdateConsoleWindow( void );
FBCALL void fb_hConvertToConsole( int *left, int *top, int *right,
	                              int *bottom );
FBCALL void fb_hConvertFromConsole( int *left, int *top, int *right,
	                                int *bottom );
FBCALL void fb_ConsoleLocateRaw( int row, int column, int cursor );
FBCALL void fb_ConsoleGetScreenSize( int *columns, int *rows );
void fb_ConsoleGetMaxWindowSize( int *columns, int *rows );
void fb_ConsoleGetScreenSizeEx( HANDLE console, int *columns, int *rows );
int fb_ConsoleGetRawYEx( HANDLE console );
int fb_ConsoleGetRawXEx( HANDLE console );
void fb_ConsoleGetRawXYEx( HANDLE console, int *column, int *row );
void fb_ConsoleLocateRawEx( HANDLE console, int row, int column, int cursor );
unsigned int fb_ConsoleGetColorAttEx( HANDLE console );
void fb_ConsoleClearViewRawEx( HANDLE console, int left, int top, int right,
	                           int bottom );
void fb_hConsoleGetWindow( int *left, int *top, int *columns, int *rows );
int fb_ConsoleProcessEvents( void );
int fb_hConsoleGetKey( int full );
int fb_hConsolePeekKey( int full );
void fb_hConsolePutBackEvents( void );
HANDLE fb_hConsoleGetHandle( int is_input );
void fb_hConsoleResetHandles( void );
int fb_ConsoleGetRawX( void );
int fb_ConsoleGetRawY( void );
HANDLE fb_hConsoleCreateBuffer( void );
int fb_ConsoleHasFocus( void );

/*
    Windows CE has no desktop console handles.  These macros retain the
    signatures shared with the Win32 runtime while always selecting the
    process-local logical page.
*/
#define __fb_in_handle  fb_hConsoleGetHandle( TRUE )
#define __fb_out_handle fb_hConsoleGetHandle( FALSE )

#define FB_CON_CORRECT_POSITION() ((void)0)
#define FB_CONSOLE_WINDOW_EMPTY() FALSE

#endif

/* end of wince/fb_private_console.h */
