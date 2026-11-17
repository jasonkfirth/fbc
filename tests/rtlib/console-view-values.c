/*
 * Project: FreeBASIC runtime regression
 * File: console-view-values.c
 * Purpose: Verify console view bounds, queries and packed row results.
 * Responsibilities: Exercise the production entry points with deterministic
 * console callbacks, including INT_MAX query state and high packed rows.
 * Native terminal rendering belongs to the platform driver tests.
 */
#include "console/io_view.c"
#include <limits.h>
#include <stdint.h>

/* This serial observer owns the fake console state and callback counters. */
static int height, current_top, current_bottom;
static int stored_top, stored_bottom, located_row;
static unsigned stores, updates, locations;
static unsigned long cases, checks;

static void check( int condition )
{
	checks++;
	if( !condition ) {
		fprintf( stderr, "CONSOLE_VIEW_FAIL case=%lu check=%lu\n", cases, checks );
		exit(1);
	}
}

FBCALL void fb_GetSize( int *columns, int *rows )
{
	check( columns == NULL && rows != NULL );
	*rows = height;
}

int fb_ConsoleGetTopRow( void )
{
	return current_top;
}

int fb_ConsoleGetBotRow( void )
{
	return current_bottom;
}

void fb_ConsoleSetTopBotRows( int top, int bottom )
{
	stores++;
	stored_top = top;
	stored_bottom = bottom;
}

FBCALL void fb_ViewUpdate( void )
{
	updates++;
}

FBCALL int fb_Locate( int row, int column, int cursor, int start, int stop )
{
	check( column == 1 && cursor == -1 && start == 0 && stop == 0 );
	locations++;
	located_row = row;
	return 0;
}

static void view_case( int top, int bottom, int cursor, int public_entry )
{
	/* The independent oracle keeps query arithmetic wide until validation.
	   The result packs row bits modulo 32 bits, preserving the legacy ABI. */
	int64_t maximum = height == 0 ? FB_SCRN_DEFAULT_HEIGHT : height;
	int64_t expected_top = top < 0 ? (int64_t)current_top + 1 : top;
	int64_t expected_bottom = bottom < 0 ? (int64_t)current_bottom + 1 : bottom;
	if( top == 0 ) expected_top = 1;
	if( bottom == 0 ) expected_bottom = maximum;
	int valid = expected_top >= 1 && expected_bottom >= expected_top &&
		expected_bottom <= maximum && expected_top <= INT_MAX && expected_bottom <= INT_MAX;
	int update = valid && (top >= 0 || bottom >= 0);
	if( !valid ) expected_top = expected_bottom = 0;
	uint32_t packed = (uint32_t)expected_top | ((uint32_t)expected_bottom << 16);
	stores = updates = locations = 0;
	stored_top = stored_bottom = located_row = INT_MIN;
	cases++;
	int actual = public_entry ? fb_ConsoleView(top, bottom) : fb_ConsoleViewEx(top, bottom, cursor);
	check( (uint32_t)actual == packed );
	check( stores == (unsigned)update && updates == (unsigned)update );
	check( locations == (unsigned)(update && (public_entry || cursor)) );
	if( update ) {
		check( stored_top == expected_top - 1 && stored_bottom == expected_bottom - 1 );
		if( public_entry || cursor ) check( located_row == expected_top );
	}
}

int main( void )
{
	static const int heights[] = { INT_MIN, -1, 0, 1, 25, 32768, 65536, INT_MAX };
	static const int inputs[] = { INT_MIN, -1, 0, 1, 25, 32768, 65535, 65536, INT_MAX };
	static const int queries[] = { INT_MIN, -1, 0, 24, 65535, INT_MAX };
	for( size_t h = 0; h < sizeof(heights) / sizeof(heights[0]); h++ )
		for( size_t t = 0; t < sizeof(queries) / sizeof(queries[0]); t++ )
			for( size_t b = 0; b < sizeof(queries) / sizeof(queries[0]); b++ ) {
				height = heights[h];
				current_top = queries[t];
				current_bottom = queries[b];
				for( size_t i = 0; i < sizeof(inputs) / sizeof(inputs[0]); i++ )
					for( size_t j = 0; j < sizeof(inputs) / sizeof(inputs[0]); j++ ) {
						view_case(inputs[i], inputs[j], FALSE, FALSE);
						view_case(inputs[i], inputs[j], TRUE, FALSE);
						view_case(inputs[i], inputs[j], TRUE, TRUE);
					}
			}
	printf( "CONSOLE_VIEW_PASS cases=%lu checks=%lu\n", cases, checks );
	return 0;
}

/* end of console-view-values.c */
