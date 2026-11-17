/*
 * Project: FreeBASIC runtime regression tests
 * File: linux-filesystem-sizes.c
 * Purpose: Check filesystem, locale and memory-query count conversions.
 * Responsibilities: Exercise actual runtime entry points with bounded syscall
 * results, temporary-string callbacks and an exclusively owned directory.
 * Public BASIC execution and complete native libraries have separate controls.
 */
#define _GNU_SOURCE 1
#include "fb.h"
#include <errno.h>
#include <langinfo.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>

size_t __real_strlen( const char *text );
size_t __wrap_strlen( const char *text );
ssize_t __real_readlink( const char *path, char *buffer, size_t size );
ssize_t __wrap_readlink( const char *path, char *buffer, size_t size );
char *__real_getcwd( char *buffer, size_t size );
char *__wrap_getcwd( char *buffer, size_t size );
long __wrap_get_avphys_pages( void );
long __wrap_sysconf( int name );
char *__wrap_nl_langinfo( nl_item item );

FB_RTLIB_CTX fb_ctx;
static unsigned int checks;
static int string_lock_depth, allocations, locale_calls, link_calls, cwd_calls;
static int fake_link, link_error, fake_cwd, cwd_error;
static char link_data[128] = "/one/two/program";
static char cwd_data[] = "/one/two";
static char locale_data[] = "Calendar";
static const char *large_length_for;
static nl_item expected_item;
static long pages, page_size;
static FBSTRING result;
static char result_data[4096];
static void *directory_context;
static size_t directory_context_size;
static FB_TLS_DESTRUCTOR directory_destructor;

static void check_at( int condition, int line )
{
	checks++;
	if( !condition ) {
		fprintf( stderr, "LINUX_FILESYSTEM_SIZES_FAIL check=%u line=%d\n", checks, line );
		exit( 1 );
	}
}
#define check(condition) check_at( (condition), __LINE__ )

size_t __wrap_strlen( const char *text )
{
	return text == large_length_for ? SIZE_MAX : __real_strlen( text );
}

ssize_t __wrap_readlink( const char *path, char *buffer, size_t size )
{
	size_t length;
	if( !fake_link )
		return __real_readlink( path, buffer, size );
	link_calls++;
	if( link_error ) {
		errno = ENOENT;
		return -1;
	}
	length = __real_strlen( link_data );
	if( length > size )
		length = size;
	memcpy( buffer, link_data, length );
	return (ssize_t)length;
}

char *__wrap_getcwd( char *buffer, size_t size )
{
	size_t length;
	if( !fake_cwd )
		return __real_getcwd( buffer, size );
	cwd_calls++;
	length = sizeof(cwd_data) - 1;
	if( cwd_error || size <= length ) {
		errno = cwd_error ? ENOENT : ERANGE;
		return NULL;
	}
	memcpy( buffer, cwd_data, length + 1 );
	return buffer;
}

long __wrap_get_avphys_pages( void ) { return pages; }
long __wrap_sysconf( int name ) { check( name == _SC_PAGE_SIZE ); return page_size; }
char *__wrap_nl_langinfo( nl_item item )
{
	check( item == expected_item );
	locale_calls++;
	return locale_data;
}

void fb_Lock( void ) { }
void fb_Unlock( void ) { }
void fb_StrLock( void ) { check( string_lock_depth++ == 0 ); }
void fb_StrUnlock( void ) { check( --string_lock_depth == 0 ); }
int fb_ErrorSetNum( int error ) { return error; }
int fb_hStrDelTemp( FBSTRING_ACCESS *text ) { (void)text; return 0; }
int fb_hStrDelTemp_NoLock( FBSTRING_ACCESS *text )
{
	check( string_lock_depth == 1 );
	return fb_hStrDelTemp( text );
}

FBSTRING_ACCESS *fb_hStrAllocTemp( FBSTRING_ACCESS *text, ssize_t size )
{
	(void)text;
	allocations++;
	if( size < 0 || (size_t)size >= sizeof(result_data) )
		return NULL;
	result.data = result_data;
	result.len = result.size = size;
	memset( result_data, 0, (size_t)size + 1 );
	return &result;
}

FBSTRING_ACCESS *fb_hStrAllocTemp_NoLock( FBSTRING_ACCESS *text, ssize_t size )
{
	check( string_lock_depth == 1 );
	return fb_hStrAllocTemp( text, size );
}

void fb_hStrCopy( char *destination, const char *source, ssize_t bytes )
{
	check( bytes >= 0 && (size_t)bytes < sizeof(result_data) );
	memcpy( destination, source, (size_t)bytes );
	destination[bytes] = '\0';
}

void *fb_TlsGetCtx( int key, size_t size, FB_TLS_DESTRUCTOR destructor )
{
	check( key == FB_TLSKEY_DIR && size < 8192 );
	if( directory_context == NULL ) {
		directory_context = calloc( 1, size );
		check( directory_context != NULL );
		directory_context_size = size;
		directory_destructor = destructor;
	}
	check( size == directory_context_size );
	return directory_context;
}

static FBSTRING descriptor( char *text )
{
	FBSTRING value = { .data = text, .len = (ssize_t)__real_strlen(text), .size = 0 };
	return value;
}

static void executable_paths( void )
{
	char guarded[132];
	size_t capacity, length = __real_strlen( link_data );
	int kind;
	fake_link = TRUE;
	for( kind = 0; kind < 2; kind++ ) {
		for( capacity = 0; capacity <= length + 3; capacity++ ) {
			char *value;
			memset( guarded, 0x5A, sizeof(guarded) );
			value = kind == 0 ? fb_hGetExeName(guarded + 1, (ssize_t)capacity) :
			    fb_hGetExePath(guarded + 1, (ssize_t)capacity);
			check( guarded[0] == 0x5A && guarded[capacity + 1] == 0x5A );
			if( capacity <= length ) {
				check( value == NULL );
				if( capacity != 0 )
					check( guarded[1] == '\0' );
			} else {
				check( value != NULL );
				check( strcmp(kind == 0 ? value : guarded + 1, kind == 0 ? "program" : "/one/two") == 0 );
			}
		}
	}
	strcpy( link_data, "/program" );
	check( fb_hGetExePath(guarded, sizeof(guarded)) != NULL && strcmp(guarded, "/") == 0 );
	link_error = TRUE;
	check( fb_hGetExeName(guarded, sizeof(guarded)) == NULL && guarded[0] == '\0' );
	check( fb_hGetExePath(guarded, sizeof(guarded)) == NULL && guarded[0] == '\0' );
	link_error = FALSE;
	fake_link = FALSE;
}

static void null_paths( void )
{
	check( fb_hGetExeName(NULL, 16) == NULL );
	check( fb_hGetExePath(NULL, 16) == NULL );
}

static void current_directory( void )
{
	char guarded[32];
	size_t capacity;
	fake_cwd = TRUE;
	cwd_calls = 0;
	check( fb_hGetCurrentDir(NULL, 10) == 0 );
	check( fb_hGetCurrentDir(guarded, -1) == 0 && cwd_calls == 0 );
	for( capacity = 0; capacity < sizeof(cwd_data) + 2; capacity++ ) {
		ssize_t length;
		memset( guarded, 0x5A, sizeof(guarded) );
		length = fb_hGetCurrentDir( guarded + 1, (ssize_t)capacity );
		check( guarded[0] == 0x5A && guarded[capacity + 1] == 0x5A );
		if( capacity < sizeof(cwd_data) )
			check( length == 0 );
		else
			check( length == (ssize_t)sizeof(cwd_data) - 1 && strcmp(guarded + 1, cwd_data) == 0 );
	}
	cwd_error = TRUE;
	check( fb_hGetCurrentDir(guarded, sizeof(guarded)) == 0 );
	cwd_error = FALSE;
	fake_cwd = FALSE;
}

static void memory_counts( void )
{
	static const long page_counts[] = { -1, 0, 1, 1, 1, LONG_MAX, LONG_MAX, LONG_MAX };
	static const long page_sizes[] = { 4096, -1, -1, 0, 4096, 1, 2, 3 };
	size_t expected[] = { 0, 0, 0, 0, 4096, (size_t)LONG_MAX, SIZE_MAX - 1, SIZE_MAX };
	size_t index;
	for( index = 0; index < sizeof(page_counts) / sizeof(page_counts[0]); index++ ) {
		pages = page_counts[index];
		page_size = page_sizes[index];
		check( fb_GetMemAvail(0) == expected[index] );
		check( fb_GetMemAvail(-1) == expected[index] );
		check( fb_GetMemAvail(INT_MAX) == expected[index] );
	}
}

static void locale_names( void )
{
	int short_name, index;
	for( short_name = 0; short_name <= 1; short_name++ ) {
		for( index = 1; index <= 12; index++ ) {
			expected_item = (nl_item)((short_name ? ABMON_1 : MON_1) + index - 1);
			check( fb_DrvIntlGetMonthName(index, short_name) == &result );
			check( strcmp(result.data, locale_data) == 0 && result.len == (ssize_t)sizeof(locale_data) - 1 );
		}
		for( index = 1; index <= 7; index++ ) {
			expected_item = (nl_item)((short_name ? ABDAY_1 : DAY_1) + index - 1);
			check( fb_DrvIntlGetWeekdayName(index, short_name) == &result );
			check( strcmp(result.data, locale_data) == 0 );
		}
	}
	locale_calls = 0;
	check( fb_DrvIntlGetMonthName(0, FALSE) == NULL );
	check( fb_DrvIntlGetMonthName(13, FALSE) == NULL );
	check( fb_DrvIntlGetWeekdayName(0, FALSE) == NULL );
	check( fb_DrvIntlGetWeekdayName(8, FALSE) == NULL && locale_calls == 0 );
	large_length_for = locale_data;
	allocations = 0;
	expected_item = MON_1;
	check( fb_DrvIntlGetMonthName(1, FALSE) == NULL && allocations == 0 );
	expected_item = DAY_1;
	check( fb_DrvIntlGetWeekdayName(1, FALSE) == NULL && allocations == 0 );
	large_length_for = NULL;
}

static void directory_names( void )
{
	char root[] = "filesystem-sizes-XXXXXX";
	char first[128], second[128], pattern[128], empty[] = "";
	FBSTRING spec;
	FBSTRING_ACCESS *value;
	FILE *stream;
	int attributes, found = 0;
	check( mkdtemp(root) != NULL );
	check( snprintf(first, sizeof(first), "%s/a.txt", root) < (int)sizeof(first) );
	check( snprintf(second, sizeof(second), "%s/b.txt", root) < (int)sizeof(second) );
	check( snprintf(pattern, sizeof(pattern), "%s/*.txt", root) < (int)sizeof(pattern) );
	stream = fopen( first, "wb" );
	check( stream != NULL && fclose(stream) == 0 );
	stream = fopen( second, "wb" );
	check( stream != NULL && fclose(stream) == 0 );
	spec = descriptor( pattern );
	value = fb_Dir( &spec, 0x37, &attributes );
	while( value != &fb_ctx.null_desc ) {
		check( strcmp(value->data, "a.txt") == 0 || strcmp(value->data, "b.txt") == 0 );
		check( attributes == 0x20 );
		found++;
		check( found <= 2 );
		spec = descriptor( empty );
		value = fb_Dir( &spec, 0x37, &attributes );
	}
	check( found == 2 && attributes == 0 );
	spec = descriptor( first );
	large_length_for = strrchr( first, '/' ) + 1;
	allocations = 0;
	check( fb_Dir(&spec, 0x37, &attributes) == &fb_ctx.null_desc && allocations == 0 );
	large_length_for = NULL;
	check( chmod(first, 0400) == 0 );
	spec = descriptor( first );
	check( fb_Dir(&spec, 0x37, &attributes) == &result && attributes == 0x21 );
	check( unlink(first) == 0 && unlink(second) == 0 && rmdir(root) == 0 );
	if( directory_context != NULL ) {
		directory_destructor( directory_context );
		free( directory_context );
		directory_context = NULL;
	}
}

int main( int argc, char **argv )
{
	const char *group = argc > 1 ? argv[1] : "all";
	if( strcmp(group, "all") == 0 || strcmp(group, "paths") == 0 ) executable_paths();
	if( strcmp(group, "all") == 0 || strcmp(group, "null") == 0 ) null_paths();
	if( strcmp(group, "all") == 0 || strcmp(group, "cwd") == 0 ) current_directory();
	if( strcmp(group, "all") == 0 || strcmp(group, "memory") == 0 ) memory_counts();
	if( strcmp(group, "all") == 0 || strcmp(group, "locale") == 0 ) locale_names();
	if( strcmp(group, "all") == 0 || strcmp(group, "directory") == 0 ) directory_names();
	check( string_lock_depth == 0 );
	printf( "LINUX_FILESYSTEM_SIZES_PASS checks=%u\n", checks );
	return 0;
}

/* end of linux-filesystem-sizes.c */
