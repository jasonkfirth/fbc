/*
 * Project: FreeBASIC runtime tests
 * File: str-instrrev-runtime.c
 * Purpose: Check reverse substring searches through the runtime entry point.
 * Responsibilities: Cover start positions, byte strings and bounded candidates.
 * Scope: String searching and descriptor preservation.
 */
#ifdef FB_INSTRREV_SOURCE
#include FB_INSTRREV_SOURCE
#else
#include "fb.h"
#endif

static int checks;

static void check( int condition )
{
	++checks;
	if( !condition ) {
		fprintf( stderr, "STR_INSTRREV_RUNTIME_FAIL check=%d\n", checks );
		exit( 1 );
	}
}

static ssize_t reference( const FBSTRING *text, const FBSTRING *pattern, ssize_t start )
{
	ssize_t position;
	if( text->len == 0 || pattern->len == 0 || pattern->len > text->len ||
	    start == 0 || start > text->len )
		return 0;
	position = text->len - pattern->len;
	if( start > 0 && start - 1 < position )
		position = start - 1;
	while( position >= 0 ) {
		if( memcmp( text->data + position, pattern->data, (size_t)pattern->len ) == 0 )
			return position + 1;
		--position;
	}
	return 0;
}

static void fill( char *bytes, unsigned int value, ssize_t length )
{
	ssize_t i;
	for( i=0; i<length; ++i ) {
		/* Include embedded NULs and high bytes in every length's corpus. */
		bytes[i] = (value & 1) ? (char)0xff : '\0';
		value >>= 1;
	}
}

int main( int argc, char **argv )
{
	char text_bytes[8], pattern_bytes[5];
	FBSTRING text = {text_bytes, 0, sizeof( text_bytes )};
	FBSTRING pattern = {pattern_bytes, 0, sizeof( pattern_bytes )};
	unsigned int text_value, pattern_value;
	ssize_t start;
	fb_Init( argc, argv, 0 );

#ifdef FB_INSTRREV_SOURCE
	/* Readable suffix bytes must stay outside the helper's stated span. */
	check( fb_hFindNaive( 1, "abba", 1, "b", 1 ) == 0 );
	check( fb_hFindNaive( -1, "abba", 4, "a", 1 ) == 0 );
	check( fb_hFindNaive( 0, "abba", 0, "a", 1 ) == 0 );
	check( fb_hFindNaive( 0, "abba", 4, "", 0 ) == 0 );
#endif
	for( text.len=0; text.len<=7; ++text.len ) {
		for( text_value=0; text_value < (1U << text.len); ++text_value ) {
			fill( text_bytes, text_value, text.len );
			for( pattern.len=0; pattern.len<=4; ++pattern.len ) {
				for( pattern_value=0; pattern_value < (1U << pattern.len); ++pattern_value ) {
					fill( pattern_bytes, pattern_value, pattern.len );
					for( start=-2; start<=text.len+1; ++start ) {
						check( fb_StrInstrRev( &text, &pattern, start ) ==
						       reference( &text, &pattern, start ) );
					}
				}
			}
		}
	}
	check( fb_StrInstrRev( NULL, &pattern, -1 ) == 0 );
	check( fb_StrInstrRev( &text, NULL, -1 ) == 0 );
	text.data = NULL;
	check( fb_StrInstrRev( &text, &pattern, -1 ) == 0 );
	printf( "STR_INSTRREV_RUNTIME_PASS checks=%d\n", checks );
	return 0;
}

/* end of str-instrrev-runtime.c */
