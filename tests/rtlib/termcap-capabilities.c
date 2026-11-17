/*
 * Project: FreeBASIC Unix runtime tests
 * File: termcap-capabilities.c
 * Purpose: Qualify capability IDs against const and legacy mutable prototypes.
 * Responsibilities: Preserve caller bytes and forward lookup/area results.
 * Actual terminal database behavior belongs to the console PTY regressions.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fb_private_termcap.h"

static unsigned long checks;
static char reply[] = "terminal-sequence";
static char expected[3];
static char **expected_area;

static void check( int condition )
{
	checks++;
	if( !condition )
		exit(1);
}

#ifdef FB_TEST_MUTABLE_TERMCAP
char *tgetstr( char *id, char **area )
#else
char *tgetstr( const char *id, char **area )
#endif
{
	check( strcmp( id, expected ) == 0 && area == expected_area );
#ifdef FB_TEST_MUTABLE_TERMCAP
	id[0] = 'x';
#endif
	if( area != NULL )
		*area = reply;
	return reply;
}

#ifdef FB_TEST_MUTABLE_TERMCAP
int tgetflag( char *id )
#else
int tgetflag( const char *id )
#endif
{
	check( strcmp( id, expected ) == 0 );
#ifdef FB_TEST_MUTABLE_TERMCAP
	id[1] = 'x';
#endif
	return 7;
}

int main( void )
{
	static const char *const ids[] = { "pc", "am", "cm", "ho", "cs", "cl", "@7", "k;", "F2" };
	for( size_t i = 0; i < sizeof(ids) / sizeof(ids[0]); i++ ) {
		char *area = NULL;
		memcpy( expected, ids[i], sizeof(expected) );
		expected_area = &area;
		check( fb_hTermcapGetStr( ids[i], &area ) == reply );
		check( area == reply );
		expected_area = NULL;
		check( fb_hTermcapGetStr( ids[i], NULL ) == reply );
		check( fb_hTermcapGetFlag( ids[i] ) == 7 );
		check( strcmp( ids[i], expected ) == 0 );
	}
	fprintf( stderr, "TERMCAP_CAPABILITIES_PASS checks=%lu\n", checks );
	return 0;
}

/* end of termcap-capabilities.c */
