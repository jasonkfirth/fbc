/*
 * Project: FreeBASIC compiler tests
 * File: semantic-output-test.c
 * Purpose: Exercise semantic publication under deterministic I/O failures.
 * Responsibilities: Inject write, flush, close, and rename failures; check cleanup.
 * This file intentionally does NOT contain: semantic records or compiler parsing.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

enum FAILURE { NO_FAILURE, WRITE_FAILURE, FLUSH_FAILURE, CLOSE_FAILURE, RENAME_FAILURE };
static enum FAILURE failure;
static size_t test_write( const void *, size_t, size_t, FILE * );
static int test_flush( FILE * );
static int test_close( FILE * );
static int test_rename( const char *, const char * );

/* Replace only the writer's calls. The injected close still closes the real
 * stream, which verifies that the failure path never closes it a second time. */
#define fwrite test_write
#define fflush test_flush
#define fclose test_close
#define rename test_rename
#include "semantic-output.c"
#undef fwrite
#undef fflush
#undef fclose
#undef rename

static size_t test_write( const void *data, size_t size, size_t count, FILE *stream )
{
	if( failure == WRITE_FAILURE ) { errno = ENOSPC; return 0; }
	return fwrite( data, size, count, stream );
}

static int test_flush( FILE *stream )
{
	int status = fflush( stream );
	return failure == FLUSH_FAILURE ? EOF : status;
}

static int test_close( FILE *stream )
{
	int status = fclose( stream );
	return failure == CLOSE_FAILURE ? EOF : status;
}

static int test_rename( const char *source, const char *destination )
{
	if( failure == RENAME_FAILURE ) { errno = EACCES; return -1; }
	return rename( source, destination );
}

static int check_contents( const char *expected )
{
	char bytes[64];
	FILE *stream = fopen( "output.tsv", "rb" );
	size_t length;
	if( stream == NULL ) return 0;
	length = fread( bytes, 1, sizeof( bytes ), stream );
	fclose( stream );
	return length == strlen( expected ) && memcmp( bytes, expected, length ) == 0;
}

int main( void )
{
	const char *previous = "previous output\n", *replacement = "replacement output\n";
	void *output;
	FILE *stream;
	int status;
	for( failure = NO_FAILURE; failure <= RENAME_FAILURE; ++failure ) {
		stream = fopen( "output.tsv", "wb" );
		if( stream == NULL || fputs( previous, stream ) < 0 || fclose( stream ) != 0 ) return 1;
		output = fbSemanticOutputOpen( "output.tsv" );
		if( output == NULL ) return 2;
		status = fbSemanticOutputWrite( output, replacement, strlen( replacement ) );
		if( (failure == WRITE_FAILURE) == (status != 0) ) return 3;
		status = fbSemanticOutputFinish( output, 1 );
		if( (failure == NO_FAILURE) != (status != 0) ) return 4;
		if( !check_contents( failure == NO_FAILURE ? replacement : previous ) ) return 5;
	}
	puts( "semantic publication failures passed" );
	return 0;
}

/* end of semantic-output-test.c */
