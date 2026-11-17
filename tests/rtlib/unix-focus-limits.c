/*
 * Project: FreeBASIC runtime regression
 * File: unix-focus-limits.c
 * Purpose: Check focus reference limits without a desktop connection.
 * Responsibilities: Exercise exit underflow guards and saturated init state.
 * Display ownership and ordinary shared lifetimes have separate driver tests.
 */
#include "drivers/unix/io_xfocus.c"
#include <limits.h>

/* Inclusion exposes the driver's private counter to this serial observer. */
static unsigned loads, unloads, checks;

FB_DYLIB fb_hDynLoad( const char *name, const char *const *symbols, void **slots )
{
	(void)name;
	(void)symbols;
	(void)slots;
	loads++;
	return NULL;
}

void fb_hDynUnload( FB_DYLIB *library )
{
	(void)library;
	unloads++;
}

static void check( int condition )
{
	checks++;
	if( !condition ) {
		fprintf( stderr, "UNIX_FOCUS_LIMITS_FAIL check=%u\n", checks );
		exit(1);
	}
}

int main( void )
{
	static const unsigned values[] = { 0, 1, 2, INT_MAX, (unsigned)INT_MAX + 1u, UINT_MAX };
	for( size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++ ) {
		ref_count = values[i];
		fb_hXTermExitFocus();
		check( ref_count == (values[i] > 0 ? values[i] - 1u : values[i]) );
		check( loads == 0 && unloads == 0 && display == NULL && xlib == NULL );
	}
	ref_count = UINT_MAX;
	check( fb_hXTermInitFocus() == -1 && ref_count == UINT_MAX );
	check( loads == 0 && unloads == 0 );
	ref_count = 0;
	check( fb_hXTermInitFocus() == -1 && ref_count == 1 );
	check( loads == 1 && unloads == 0 );
	fb_hXTermExitFocus();
	check( ref_count == 0 && loads == 1 && unloads == 0 );
	printf( "UNIX_FOCUS_LIMITS_PASS checks=%u\n", checks );
	return 0;
}

/* end of unix-focus-limits.c */
