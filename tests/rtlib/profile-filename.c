/*
 * Project: FreeBASIC runtime tests
 * File: profile-filename.c
 * Purpose: Check profile-name suffix handling at the fixed path-buffer limit.
 * Responsibilities: Exercise production filename selection with a fake exe name.
 * This file does not create profiling reports or access the filesystem.
 */
#include "../../src/rtlib/profiling/profile.c"

static ssize_t executable_length;
static int checks;

FBCALL int fb_ErrorSetNum( int error ) { return error; }

char *fb_hGetExeName( char *destination, ssize_t capacity )
{
	if( executable_length < 0 )
		return NULL;
	if( executable_length >= capacity )
		abort();
	memset( destination, 'x', (size_t)executable_length );
	destination[executable_length] = '\0';
	return destination;
}

static void check( int condition )
{
	++checks;
	if( !condition ) {
		fprintf( stderr, "PROFILE_FILENAME_FAIL check=%d\n", checks );
		exit( 1 );
	}
}

int main( void )
{
	FB_PROFILER_GLOBAL profile = {0};
	char result[PROFILER_MAX_PATH + 1];
	int status;

	fb_profiler = &profile;
	for( executable_length = -1; executable_length < PROFILER_MAX_PATH - 1; ++executable_length ) {
		memset( result, 0x5a, sizeof( result ) );
		status = fb_ProfileGetFileName( result, PROFILER_MAX_PATH );
		/* The existing filename-copy contract rejects a 1023-byte name even
		   when the local 1024-byte buffer can hold it and its terminator. */
		if( executable_length == PROFILER_MAX_PATH - (ssize_t)sizeof( DEFAULT_PROFILE_EXT ) ) {
			check( status == FB_RTERROR_ILLEGALFUNCTIONCALL );
		} else {
			check( status == FB_RTERROR_OK );
			if( executable_length < 0 || executable_length > PROFILER_MAX_PATH - (ssize_t)sizeof( DEFAULT_PROFILE_EXT ) ) {
				check( strcmp( result, DEFAULT_PROFILE_FILE ) == 0 );
			} else {
				check( strlen( result ) == (size_t)executable_length + sizeof( DEFAULT_PROFILE_EXT ) - 1 );
				check( strcmp( result + executable_length, DEFAULT_PROFILE_EXT ) == 0 );
			}
		}
		check( result[PROFILER_MAX_PATH] == 0x5a );
	}
	check( fb_ProfileGetFileName( result, 0 ) == FB_RTERROR_ILLEGALFUNCTIONCALL );
	check( fb_ProfileGetFileName( NULL, 1 ) == FB_RTERROR_ILLEGALFUNCTIONCALL );
	fb_profiler = NULL;
	printf( "PROFILE_FILENAME_PASS checks=%d\n", checks );
	return 0;
}

/* end of profile-filename.c */
