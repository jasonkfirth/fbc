/* FreeBASIC Android tests: app-paths.c.
 * Purpose: Check the APK asset path override without changing CLI behavior.
 * Responsibilities: Ownership, bounded copies and normal proc-fs fallback.
 * This test does not create files or change a real application's HOME. */
#include "fb.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

void fb_hAndroidSetAppFilesPath( const char *path );

int main( void )
{
	char output[PATH_MAX];
	char original[PATH_MAX];
	char borrowed[] = "/data/user/0/example/files";
	char too_long[PATH_MAX + 1];
	char short_output[8];
	assert( fb_hGetExePath( original, sizeof(original) ) != NULL );
	assert( original[0] == '/' );
	fb_hAndroidSetAppFilesPath( borrowed );
	borrowed[1] = 'X';
	assert( fb_hGetExePath( output, sizeof(output) ) == output );
	assert( strcmp( output, "/data/user/0/example/files" ) == 0 );
	memset( short_output, 0x73, sizeof(short_output) );
	assert( fb_hGetExePath( short_output, 4 ) == NULL );
	assert( short_output[0] == 0 && short_output[4] == 0x73 );
	assert( fb_hGetExePath( NULL, 30 ) == NULL );
	assert( fb_hGetExePath( output, 0 ) == NULL );
	fb_hAndroidSetAppFilesPath( "relative" );
	assert( fb_hGetExePath( output, sizeof(output) ) != NULL );
	assert( strcmp( output, original ) == 0 );
	memset( too_long, 'a', sizeof(too_long) );
	too_long[0] = '/'; too_long[PATH_MAX] = 0;
	fb_hAndroidSetAppFilesPath( too_long );
	assert( fb_hGetExePath( output, sizeof(output) ) != NULL );
	assert( strcmp( output, original ) == 0 );
	puts( "ANDROID_APP_PATHS=PASS" );
	return 0;
}
/* end of app-paths.c */
