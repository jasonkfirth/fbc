/*
 * FreeBASIC runtime tests: android-provider.c
 * Purpose: Exercise the real Android provider with controlled syscall failures.
 * Responsibilities: Boot readiness, exact device reads, cleanup and public wiping.
 * This fixture does not estimate entropy or use a production account.
 * One test thread owns the mock state. --live forwards to the actual system.
 */
#include "fb_secure_random.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/system_properties.h>
#include <unistd.h>

static int mode, system_calls, opens, reads, closes, properties;
static const int test_descriptor = 47;

long __real_syscall( long number, ... );
int __real_open( const char *path, int flags, ... );
ssize_t __real_read( int descriptor, void *buffer, size_t count );
int __real_close( int descriptor );
int __real_fstat( int descriptor, struct stat *information );
int __real___system_property_get( const char *name, char *value );

long __wrap_syscall( long number, ... )
{
	va_list arguments;
	void *buffer;
	size_t count;
	unsigned int flags;
	va_start( arguments, number );
	buffer = va_arg( arguments, void * );
	count = va_arg( arguments, size_t );
	flags = va_arg( arguments, unsigned int );
	va_end( arguments );
	if( mode < 0 ) return __real_syscall( number, buffer, count, flags );
	assert( number == SYS_getrandom && flags == 1U );
	system_calls++;
	if( mode == 0 ) { memset( buffer, 0x73, count ); return (long)count; }
	errno = mode == 9 ? EAGAIN : ENOSYS;
	return -1;
}

int __wrap___system_property_get( const char *name, char *value )
{
	if( mode < 0 ) return __real___system_property_get( name, value );
	assert( strcmp( name, "sys.boot_completed" ) == 0 );
	properties++;
	strcpy( value, mode == 2 ? "0" : "1" );
	return 1;
}

int __wrap_open( const char *path, int flags, ... )
{
	if( mode < 0 ) return __real_open( path, flags );
	assert( strcmp( path, "/dev/urandom" ) == 0 );
	assert( (flags & (O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK)) == (O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK) );
	opens++;
	if( mode == 3 ) { errno = EACCES; return -1; }
	return test_descriptor;
}

int __wrap_fstat( int descriptor, struct stat *information )
{
	if( mode < 0 ) return __real_fstat( descriptor, information );
	assert( descriptor == test_descriptor );
	memset( information, 0, sizeof(*information) );
	information->st_mode = mode == 4 ? S_IFREG : S_IFCHR;
	return 0;
}

ssize_t __wrap_read( int descriptor, void *buffer, size_t count )
{
	if( mode < 0 ) return __real_read( descriptor, buffer, count );
	assert( descriptor == test_descriptor && count > 0 && count <= 256 );
	reads++;
	if( mode == 7 ) { errno = EINTR; return -1; }
	if( mode == 8 ) return (ssize_t)(count + 1);
	if( reads > 1 && mode == 5 ) { errno = EIO; return -1; }
	if( reads > 1 && mode == 6 ) return 0;
	if( count > 17 ) count = 17;
	memset( buffer, 0x73, count );
	return (ssize_t)count;
}

int __wrap_close( int descriptor )
{
	if( mode < 0 ) return __real_close( descriptor );
	assert( descriptor == test_descriptor );
	closes++;
	if( mode == 10 ) { errno = EIO; return -1; }
	return 0;
}

static void check_span( size_t count, int expected )
{
	unsigned char *buffer = malloc( count + 32 );
	size_t index;
	assert( buffer != NULL );
	memset( buffer, 0xa5, count + 32 );
	assert( fb_SecureRandom( buffer + 16, count ) == expected );
	for( index = 0; index < 16; index++ )
		assert( buffer[index] == 0xa5 && buffer[16 + count + index] == 0xa5 );
	if( expected != FB_SECURE_RANDOM_OK )
		for( index = 0; index < count; index++ ) assert( buffer[16 + index] == 0 );
	free( buffer );
}

int main( int argc, char **argv )
{
	if( argc == 2 && strcmp( argv[1], "--live" ) == 0 ) {
		mode = -1;
		check_span( 1, FB_SECURE_RANDOM_OK );
		check_span( 257, FB_SECURE_RANDOM_OK );
		check_span( FB_SECURE_RANDOM_MAX_BYTES, FB_SECURE_RANDOM_OK );
		puts( "ANDROID_SECURE_RANDOM_LIVE=PASS" );
		return 0;
	}
	for( mode = 0; mode <= 10; mode++ ) {
		int expected = FB_SECURE_RANDOM_IO_ERROR;
		system_calls = opens = reads = closes = properties = 0;
		if( mode <= 1 ) expected = FB_SECURE_RANDOM_OK;
		if( mode == 2 || mode == 9 ) expected = FB_SECURE_RANDOM_UNAVAILABLE;
		check_span( 4097, expected );
		assert( system_calls > 0 );
		if( mode == 0 || mode == 2 || mode == 9 ) assert( opens == 0 );
		if( mode == 9 ) assert( properties == 0 );
		if( mode == 1 || (mode >= 4 && mode != 9) ) assert( closes == 1 );
		if( mode == 7 ) assert( reads == 17 );
	}
	assert( fb_SecureRandom( NULL, 0 ) == FB_SECURE_RANDOM_OK );
	assert( fb_SecureRandom( NULL, 1 ) == FB_SECURE_RANDOM_INVALID_ARGUMENT );
	puts( "ANDROID_SECURE_RANDOM_FAULTS=PASS cases=11" );
	return 0;
}
/* end of android-provider.c */
