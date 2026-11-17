/*
 * Project: FreeBASIC Unix runtime tests
 * File: unix-driver-contracts.c
 * Purpose: Verify focus ownership and unsupported console callback contracts.
 * Responsibilities: Exercise real driver entry points with deterministic X11.
 * Desktop display rendering belongs to platform application qualification.
 */
#include "drivers/unix/io_xfocus.c"
#include "fb_private_xfocus.h"
#include <limits.h>
#include <stddef.h>

static unsigned long checks;

static void check( int condition )
{
	checks++;
	if( !condition ) {
		fprintf( stderr, "UNIX_DRIVER_CONTRACTS_FAIL check=%lu\n", checks );
		exit(1);
	}
}

FBCALL int fb_ErrorSetNum( int error )
{
	return error;
}

#ifndef DISABLE_X11
static int fail_load, fail_open, loads, opens, closes, unloads;
static max_align_t display_token;
static int library_token;
static Window reported_window = 42;

#ifdef FB_TEST_OLD_XOPEN
static Display *test_open( char *name )
#else
_Static_assert( __builtin_types_compatible_p( XOPENDISPLAY, __typeof__(&XOpenDisplay) ),
	"The loaded XOpenDisplay signature must match Xlib" );
static Display *test_open( const char *name )
#endif
{
	check( name == NULL );
	opens++;
	if( fail_open )
		return NULL;
	return (Display *)&display_token;
}

static int test_close( Display *connection )
{
	check( connection == (Display *)&display_token );
	closes++;
	return 0;
}

static int test_focus( Display *connection, Window *window, int *revert )
{
	check( connection == (Display *)&display_token );
	*window = reported_window;
	*revert = 0;
	return 1;
}

FB_DYLIB fb_hDynLoad( const char *name, const char *const *names, void **slots )
{
	X_FUNCS *functions = (void *)slots;
	check( strcmp( name, "libX11.so" ) == 0 );
	check( strcmp( names[0], "XOpenDisplay" ) == 0 );
	check( strcmp( names[1], "XCloseDisplay" ) == 0 );
	check( strcmp( names[2], "XGetInputFocus" ) == 0 && names[3] == NULL );
	loads++;
	if( fail_load )
		return NULL;
	functions->OpenDisplay = test_open;
	functions->CloseDisplay = test_close;
	functions->GetInputFocus = test_focus;
	return &library_token;
}

void fb_hDynUnload( FB_DYLIB *library )
{
	check( *library == &library_token );
	unloads++;
	*library = NULL;
}

static void focus_case( const char *name )
{
	if( strcmp( name, "uninitialized" ) == 0 ) {
		check( fb_hXTermHasFocus() == FALSE );
		fb_hXTermExitFocus();
		check( closes == 0 && unloads == 0 );
	} else if( strcmp( name, "load-failure" ) == 0 ) {
		fail_load = TRUE;
		check( fb_hXTermInitFocus() == -1 );
		check( fb_hXTermHasFocus() == FALSE );
		fb_hXTermExitFocus();
		check( loads == 1 && opens == 0 && closes == 0 && unloads == 0 );
	} else if( strcmp( name, "open-failure" ) == 0 ) {
		fail_open = TRUE;
		check( fb_hXTermInitFocus() == -1 );
		check( loads == 1 && opens == 1 && unloads == 1 );
		check( fb_hXTermHasFocus() == FALSE );
		fb_hXTermExitFocus();
		check( closes == 0 && unloads == 1 );
	} else if( strcmp( name, "retry-load" ) == 0 || strcmp( name, "retry-open" ) == 0 ) {
		fail_load = strcmp( name, "retry-load" ) == 0;
		fail_open = !fail_load;
		check( fb_hXTermInitFocus() == -1 );
		fail_load = fail_open = FALSE;
		check( fb_hXTermInitFocus() == 0 );
		check( fb_hXTermHasFocus() != FALSE );
		fb_hXTermExitFocus();
		check( closes == 0 );
		check( fb_hXTermHasFocus() != FALSE );
		fb_hXTermExitFocus();
		check( closes == 1 );
		check( fb_hXTermHasFocus() == FALSE );
		fb_hXTermExitFocus();
		check( closes == 1 && unloads == (strcmp( name, "retry-load" ) == 0 ? 1 : 2) );
	} else {
		check( strcmp( name, "shared" ) == 0 );
		for( int i = 0; i < 1000; i++ ) {
			reported_window = 42;
			check( fb_hXTermInitFocus() == 0 );
			check( fb_hXTermInitFocus() == 0 );
			check( fb_hXTermHasFocus() != FALSE );
			reported_window = 99;
			check( fb_hXTermHasFocus() == FALSE );
			fb_hXTermExitFocus();
			check( closes == i );
			fb_hXTermExitFocus();
			check( closes == i + 1 && unloads == i + 1 );
			check( fb_hXTermHasFocus() == FALSE );
			fb_hXTermExitFocus();
		}
		check( loads == 1000 && opens == 1000 );
	}
}
#endif

int main( int argc, char **argv )
{
	static const int values[] = { INT_MIN, -1, 0, 1, INT_MAX };
	for( size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++ ) {
		check( fb_ConsolePageSet( values[i], values[i] ) == -1 );
		check( fb_ConsolePageCopy( values[i], values[i] ) == FB_RTERROR_ILLEGALFUNCTIONCALL );
		check( fb_ConsoleSetMouse( values[i], values[i], values[i], values[i] ) == FB_RTERROR_ILLEGALFUNCTIONCALL );
		check( fb_DylibSymbolByOrd( NULL, (short)i ) == NULL );
	}
#ifndef DISABLE_X11
	check( argc == 2 );
	focus_case( argv[1] );
#else
	(void)argc;
	(void)argv;
	for( int i = 0; i < 1000; i++ ) {
		check( fb_hXTermInitFocus() == 0 );
		check( fb_hXTermHasFocus() == FALSE );
		fb_hXTermExitFocus();
	}
#endif
	fprintf( stderr, "UNIX_DRIVER_CONTRACTS_PASS checks=%lu\n", checks );
	return 0;
}

/* end of unix-driver-contracts.c */
