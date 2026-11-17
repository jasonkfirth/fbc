/*
 * Project: FreeBASIC Unix runtime tests
 * File: x11-keycode-values.c
 * Purpose: Verify native Xlib signatures and scan-code table rebuilding.
 * Responsibilities: Check bounded keycodes, failed mappings and returned ownership.
 * Display rendering belongs to the platform application tests.
 */
#include "drivers/unix/scancodes_x11.c"
#include <limits.h>
#include <stddef.h>

#ifndef FB_TEST_ORIGINAL
_Static_assert( __builtin_types_compatible_p( XOPENDISPLAY, __typeof__(&XOpenDisplay) ), "XOpenDisplay type" );
_Static_assert( __builtin_types_compatible_p( XQUERYKEYMAP, __typeof__(&XQueryKeymap) ), "XQueryKeymap type" );
_Static_assert( __builtin_types_compatible_p( XGETKEYBOARDMAPPING, __typeof__(&XGetKeyboardMapping) ), "XGetKeyboardMapping type" );
#endif

static unsigned long checks;
static int minimum, maximum, mapping_count = 1, missing, calls, released;
static KeySym symbol[1];
static max_align_t display_token;

static void check( int condition )
{
	checks++;
	if( !condition ) {
		fprintf( stderr, "X11_KEYCODE_VALUES_FAIL check=%lu\n", checks );
		exit(1);
	}
}

static int display_codes( Display *connection, int *first, int *last )
{
	check( connection == (Display *)&display_token );
	*first = minimum;
	*last = maximum;
	return 1;
}

static KeySym *keyboard_mapping( Display *connection, KeyCode code, int count, int *per_code )
{
	check( connection == (Display *)&display_token && count == 1 );
	check( (int)code == (minimum < 0 ? 0 : minimum) + calls );
	calls++;
	*per_code = mapping_count;
	return missing ? NULL : symbol;
}

static int release_mapping( void *data )
{
	check( data == symbol );
	released++;
	return 0;
}

static void rebuild( void )
{
	calls = released = 0;
	fb_hInitX11KeycodeToScancodeTb( (Display *)&display_token,
		display_codes, keyboard_mapping, release_mapping );
}

int main( int argc, char **argv )
{
	check( argc == 2 );
	minimum = maximum = 42;
	symbol[0] = XK_A;
	if( strcmp( argv[1], "normal" ) == 0 ) {
		for( size_t entry = 0; keysym_to_scancode[entry].scancode; entry++ ) {
			check( keysym_to_scancode[entry].scancode > 0 && keysym_to_scancode[entry].scancode < 128 );
			for( int code = 0; code < 256; code++ ) {
				memset( fb_x11keycode_to_scancode, 0, sizeof(fb_x11keycode_to_scancode) );
				minimum = maximum = code;
				symbol[0] = keysym_to_scancode[entry].keysym;
				rebuild();
				check( calls == 1 && released == 1 );
				check( fb_x11keycode_to_scancode[code] == keysym_to_scancode[entry].scancode );
			}
		}
	} else if( strcmp( argv[1], "null" ) == 0 ) {
		missing = TRUE;
		rebuild();
		check( calls == 1 && released == 0 );
		check( fb_x11keycode_to_scancode[42] == 0 );
	} else if( strcmp( argv[1], "zero-count" ) == 0 ) {
		symbol[0] = XK_a;
		for( int count = -1; count <= 0; count++ ) {
			mapping_count = count;
			rebuild();
			check( calls == 1 && released == 1 );
			check( fb_x11keycode_to_scancode[42] == 0 );
		}
	} else if( strcmp( argv[1], "range" ) == 0 ) {
		minimum = INT_MIN;
		maximum = INT_MAX;
		rebuild();
		check( calls == 256 && released == 256 );
		minimum = 300;
		maximum = INT_MAX;
		rebuild();
		check( calls == 0 && released == 0 );
		minimum = INT_MIN;
		maximum = -1;
		rebuild();
		check( calls == 0 && released == 0 );
	} else {
		check( strcmp( argv[1], "stale" ) == 0 );
		memset( fb_x11keycode_to_scancode, SC_A, sizeof(fb_x11keycode_to_scancode) );
		symbol[0] = NoSymbol;
		rebuild();
		for( size_t i = 0; i < sizeof(fb_x11keycode_to_scancode); i++ )
			check( fb_x11keycode_to_scancode[i] == 0 );
		symbol[0] = (KeySym)0x7fffffffu;
		rebuild();
		check( fb_x11keycode_to_scancode[42] == 0 );
	}
	fprintf( stderr, "X11_KEYCODE_VALUES_PASS checks=%lu\n", checks );
	return 0;
}

/* end of x11-keycode-values.c */
