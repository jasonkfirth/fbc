/*
 * Project: FreeBASIC runtime regression tests
 * File: unix-transient-buffers.c
 * Purpose: Exercise Unix process and printer temporary-storage ownership.
 * Responsibilities: Check actual runtime entry points, allocation failures,
 * argument values, byte output and small-stack calls with bounded resources.
 * Printer callbacks record commands and use temporary streams.
 */
#define _GNU_SOURCE 1
#include "fb.h"
#include "drivers/unix/fb_private_console.h"
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <sys/wait.h>
#include <unistd.h>

void *__real_malloc( size_t size );
void __real_free( void *pointer );
int __real_execvp( const char *file, char *const argv[] );
FILE *__real_fopen( const char *path, const char *mode );
void *__wrap_malloc( size_t size );
void __wrap_free( void *pointer );
int __wrap_execvp( const char *file, char *const argv[] );
FILE *__wrap_popen( const char *command, const char *mode );
int __wrap_pclose( FILE *stream );
FILE *__wrap_fopen( const char *path, const char *mode );
FILE *__wrap_fopen64( const char *path, const char *mode );

static unsigned int checks;
static size_t allocations, fail_allocation, outstanding;
static void *owned[16];
static int deleted_temps, console_exits, console_inits, exec_calls;
static int lock_depth, string_lock_depth;
static int real_execution, parse_failure, probe_failure, pipe_failure;
static int protocol_freed, printer_probes, printer_opens;
static void *protocol_pointer;
static char printer_name[8] = "test";
static char printer_title[8] = "title";
static const char *expected_file;
static const char *expected_arguments[8];
static size_t expected_count;
static char command_text[512], device_text[64];
static char *self_path;

static void check_at( int condition, int line )
{
	checks++;
	if( !condition ) {
		fprintf( stderr, "UNIX_TRANSIENT_BUFFERS_FAIL check=%u line=%d\n", checks, line );
		exit( 1 );
	}
}

#define check(condition) check_at( (condition), __LINE__ )

static void reset( void )
{
	check( outstanding == 0 );
	allocations = fail_allocation = 0;
	deleted_temps = console_exits = console_inits = exec_calls = 0;
	parse_failure = probe_failure = pipe_failure = 0;
	protocol_freed = printer_probes = printer_opens = 0;
	strcpy( printer_name, "test" );
	strcpy( printer_title, "title" );
	command_text[0] = device_text[0] = '\0';
}

void *__wrap_malloc( size_t size )
{
	void *pointer;
	size_t slot;
	allocations++;
	/* A failed size test must finish before requesting large physical memory. */
	if( allocations == fail_allocation || size > 1024 * 1024 )
		return NULL;
	pointer = __real_malloc( size );
	if( pointer != NULL ) {
		for( slot = 0; slot < sizeof(owned) / sizeof(owned[0]); slot++ ) {
			if( owned[slot] == NULL ) {
				owned[slot] = pointer;
				outstanding++;
				return pointer;
			}
		}
		check( FALSE );
	}
	return pointer;
}

void __wrap_free( void *pointer )
{
	size_t slot;
	if( pointer == protocol_pointer && pointer != NULL ) {
		protocol_freed++;
		protocol_pointer = NULL;
	}
	for( slot = 0; slot < sizeof(owned) / sizeof(owned[0]); slot++ ) {
		if( pointer != NULL && owned[slot] == pointer ) {
			owned[slot] = NULL;
			outstanding--;
			break;
		}
	}
	__real_free( pointer );
}

int fb_hStrDelTemp( FBSTRING_ACCESS *text )
{
	(void)text;
	deleted_temps++;
	return 0;
}

int fb_hStrDelTemp_NoLock( FBSTRING_ACCESS *text )
{
	check( string_lock_depth == 1 );
	return fb_hStrDelTemp( text );
}

/* Calls are serial, including the joined small-stack worker. Observe the
   runtime's lock contract without starting its background console service. */
void fb_Lock( void ) { check( lock_depth++ == 0 ); }
void fb_Unlock( void ) { check( --lock_depth == 0 ); }
void fb_StrLock( void ) { check( string_lock_depth++ == 0 ); }
void fb_StrUnlock( void ) { check( --string_lock_depth == 0 ); }
void fb_hExitConsole( void ) { check( lock_depth == 1 ); console_exits++; }
int fb_hInitConsole( void ) { check( lock_depth == 1 ); console_inits++; return TRUE; }
int fb_ErrorSetNum( int error ) { return error; }

int __wrap_execvp( const char *file, char *const argv[] )
{
	size_t index;
	if( real_execution )
		return __real_execvp( file, argv );
	exec_calls++;
	check( expected_file != NULL && strcmp(file, expected_file) == 0 );
	check( strcmp(argv[0], expected_file) == 0 );
	for( index = 0; index < expected_count; index++ )
		check( argv[index + 1] != NULL && strcmp(argv[index + 1], expected_arguments[index]) == 0 );
	check( argv[expected_count + 1] == NULL );
	errno = ENOENT;
	return -1;
}

int fb_DevLptParseProtocol( DEV_LPT_PROTOCOL **result, const char *raw, size_t length, int substitute )
{
	(void)raw;
	(void)length;
	(void)substitute;
	*result = __real_malloc( sizeof( **result ) );
	check( *result != NULL );
	protocol_pointer = *result;
	(*result)->name = printer_name;
	(*result)->title = printer_title;
	return !parse_failure;
}

FILE *__wrap_popen( const char *command, const char *mode )
{
	FILE *stream;
	if( mode[0] == 'r' ) {
		printer_probes++;
		stream = tmpfile();
		check( stream != NULL );
		fputs( "system default destination: test\n", stream );
		rewind( stream );
		return stream;
	}
	printer_opens++;
	check( strlen(command) < sizeof(command_text) );
	strcpy( command_text, command );
	return pipe_failure ? NULL : tmpfile();
}

int __wrap_pclose( FILE *stream )
{
	int result = fclose( stream );
	return probe_failure ? 256 : result;
}

FILE *__wrap_fopen( const char *path, const char *mode )
{
	(void)mode;
	check( strlen(path) < sizeof(device_text) );
	strcpy( device_text, path );
	return pipe_failure ? NULL : tmpfile();
}

FILE *__wrap_fopen64( const char *path, const char *mode )
{
	return __wrap_fopen( path, mode );
}

static FBSTRING descriptor( char *text )
{
	FBSTRING result = { .data = text, .len = (ssize_t)strlen(text), .size = 0 };
	return result;
}

static void launch( char *text, const char *const *values, size_t count )
{
	char program_name[] = "/test/program";
	FBSTRING program = descriptor( program_name );
	FBSTRING arguments = descriptor( text );
	size_t index;
	reset();
	expected_file = program.data;
	expected_count = count;
	for( index = 0; index < count; index++ )
		expected_arguments[index] = values[index];
	check( fb_ExecEx(&program, &arguments, FALSE) == -1 );
	check( exec_calls == 1 && deleted_temps == 2 );
	check( console_exits == 1 && console_inits == 1 && outstanding == 0 );
}

static void exec_values( void )
{
	const char *values[] = { "one", "two three", "", "four" };
	char program_name[] = "/test/program", argument_text[] = "one two";
	char quoted[] = "one \"two three\" \"\" four", empty[] = "", spaces[] = "  ";
	FBSTRING program = descriptor( program_name );
	FBSTRING arguments = descriptor( argument_text );
	char parsed[64];
	launch( quoted, values, 4 );
	launch( empty, values, 0 );
	launch( spaces, values, 0 );
	reset();
	expected_file = program.data;
	expected_count = 0;
	check( fb_ExecEx(&program, NULL, FALSE) == -1 && exec_calls == 1 );
	check( outstanding == 0 && deleted_temps == 2 );
	reset();
	check( fb_ExecEx(NULL, &arguments, FALSE) == -1 );
	check( deleted_temps == 2 && exec_calls == 0 && outstanding == 0 );
	check( fb_hParseArgs(parsed, "one\0two", 7) == 2 );
	check( strcmp(parsed, "one") == 0 && strcmp(parsed + 4, "two") == 0 );
	check( fb_hParseArgs(parsed, NULL, 1) == -1 );
	check( fb_hParseArgs(parsed, "one", -1) == -1 );
}

static void allocation_failures( void )
{
	char program_name[] = "/test/program", argument_text[] = "one two";
	FBSTRING program = descriptor( program_name );
	FBSTRING arguments = descriptor( argument_text );
	DEV_LPT_INFO device = { 0 };
	FB_WCHAR wide[128];
	size_t index;
	for( index = 1; index <= 2; index++ ) {
		reset();
		fail_allocation = index;
		check( fb_ExecEx(&program, &arguments, FALSE) == -1 );
		check( exec_calls == 0 && deleted_temps == 2 );
		check( console_exits == 0 && console_inits == 0 && outstanding == 0 );
	}
	reset();
	fail_allocation = 1;
	check( fb_PrinterOpen(&device, 0, "LPT:") == FB_RTERROR_OUTOFMEM );
	check( protocol_freed == 1 && printer_probes == 0 && outstanding == 0 );
	for( index = 0; index < sizeof(wide) / sizeof(wide[0]); index++ )
		wide[index] = FB_WCHAR_LITERAL('A');
	reset();
	fail_allocation = 1;
	check( fb_PrinterWriteWstr(&device, wide, 128) == FB_RTERROR_OUTOFMEM );
	check( outstanding == 0 );
	reset();
	check( fb_PrinterWriteWstr(&device, wide, SIZE_MAX) == FB_RTERROR_OUTOFMEM );
	check( allocations == 0 && outstanding == 0 );
	check( fb_PrinterWriteWstr(&device, NULL, 1) == FB_RTERROR_ILLEGALFUNCTIONCALL );
}

static void printer_values( void )
{
	DEV_LPT_INFO device = { 0 };
	FB_WCHAR wide[300];
	unsigned char bytes[1200];
	size_t length, index;
	reset();
	check( fb_PrinterOpen(&device, 0, "LPT:") == FB_RTERROR_OK );
	check( strcmp(command_text, "lp -d \"test\" -t \"title\" -s - &> /dev/null") == 0 );
	check( protocol_freed == 1 && outstanding == 0 );
	check( fb_PrinterClose(&device) == FB_RTERROR_OK && device.driver_opaque == NULL );
	reset();
	printer_name[0] = printer_title[0] = '\0';
	check( fb_PrinterOpen(&device, 0, "LPT:") == FB_RTERROR_OK );
	check( strcmp(command_text, "lp -t \"FreeBASIC document\" -s - &> /dev/null") == 0 );
	check( fb_PrinterClose(&device) == FB_RTERROR_OK && outstanding == 0 );
	reset();
	check( fb_PrinterOpen(&device, INT_MAX, "LPT:") == FB_RTERROR_OK );
	check( strcmp(device_text, "/dev/lp2147483646") == 0 );
	check( fb_PrinterClose(&device) == FB_RTERROR_OK );
	reset();
	check( fb_PrinterOpen(&device, INT_MIN, "LPT:") == FB_RTERROR_ILLEGALFUNCTIONCALL );
	check( protocol_freed == 1 && device_text[0] == '\0' );
	reset();
	probe_failure = 1;
	check( fb_PrinterOpen(&device, 0, "LPT:") == FB_RTERROR_FILENOTFOUND );
	check( protocol_freed == 1 && outstanding == 0 && printer_opens == 0 );
	reset();
	pipe_failure = 1;
	check( fb_PrinterOpen(&device, 0, "LPT:") == FB_RTERROR_FILENOTFOUND );
	check( protocol_freed == 1 && outstanding == 0 && device.driver_opaque == NULL );
	reset();
	parse_failure = 1;
	check( fb_PrinterOpen(&device, 0, "LPT:") == FB_RTERROR_ILLEGALFUNCTIONCALL );
	check( protocol_freed == 1 && outstanding == 0 );
	reset();
	check( fb_PrinterWrite(&device, NULL, 0) == FB_RTERROR_OK );
	check( fb_PrinterWriteWstr(&device, NULL, 0) == FB_RTERROR_OK );
	for( index = 0; index < 300; index++ )
		wide[index] = (FB_WCHAR)(index % 3 == 0 ? 0 : index % 3 == 1 ? 'A' : 0xE9);
	for( length = 0; length <= 300; length++ ) {
		FILE *stream = tmpfile();
		size_t expected = 0;
		check( stream != NULL );
		device.driver_opaque = stream;
		check( fb_PrinterWriteWstr(&device, wide, length) == FB_RTERROR_OK );
		check( outstanding == 0 );
		rewind( stream );
		for( index = 0; index < length; index++ ) {
			if( wide[index] == 0xE9 ) {
				check( fgetc(stream) == 0xC3 && fgetc(stream) == 0xA9 );
				expected += 2;
			} else {
				check( fgetc(stream) == (int)wide[index] );
				expected++;
			}
		}
		check( fgetc(stream) == EOF );
		rewind( stream );
		check( fread(bytes, 1, sizeof(bytes), stream) == expected );
		check( fclose(stream) == 0 );
	}
	device.driver_opaque = __real_fopen( "/dev/full", "wb" );
	check( device.driver_opaque != NULL );
	check( setvbuf(device.driver_opaque, NULL, _IONBF, 0) == 0 );
	check( fb_PrinterWriteWstr(&device, wide, 128) == FB_RTERROR_FILEIO );
	check( outstanding == 0 );
	fclose( device.driver_opaque );
}

static void real_launch( void )
{
	FBSTRING program = descriptor( self_path );
	char argument_text[] = "--exec-child \"two three\"";
	FBSTRING arguments = descriptor( argument_text );
	pid_t child;
	int status;
	reset();
	real_execution = TRUE;
	check( fb_ExecEx(&program, &arguments, TRUE) == 23 );
	check( outstanding == 0 && deleted_temps == 2 );
	child = fork();
	check( child >= 0 );
	if( child == 0 ) {
		fb_ExecEx( &program, &arguments, FALSE );
		_exit( 99 );
	}
	check( waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 23 );
	real_execution = FALSE;
}

static void *long_call( void *kind )
{
	size_t length = 40000, index;
	if( strcmp(kind, "long-exec") == 0 ) {
		length = 128000;
		char *text = __real_malloc( length + 1 );
		const char *values[] = { text };
		check( text != NULL );
		memset( text, 'A', length );
		text[length] = '\0';
		launch( text, values, 1 );
		__real_free( text );
	} else {
		FB_WCHAR *wide = __real_malloc( length * sizeof( *wide ) );
		DEV_LPT_INFO device = { 0 };
		FILE *stream = tmpfile();
		check( wide != NULL && stream != NULL );
		for( index = 0; index < length; index++ )
			wide[index] = FB_WCHAR_LITERAL('A');
		reset();
		device.driver_opaque = stream;
		check( fb_PrinterWriteWstr(&device, wide, length) == FB_RTERROR_OK );
		check( outstanding == 0 && ftell(stream) == (long)length );
		check( fclose(stream) == 0 );
		__real_free( wide );
	}
	return NULL;
}

int main( int argc, char **argv )
{
	const char *group = argc > 1 ? argv[1] : "all";
	if( strcmp(group, "--exec-child") == 0 )
		return argc == 3 && strcmp(argv[2], "two three") == 0 ? 23 : 98;
	self_path = argv[0];
	if( strcmp(group, "all") == 0 || strcmp(group, "exec") == 0 ) {
		exec_values();
		real_launch();
	}
	if( strcmp(group, "all") == 0 || strcmp(group, "failure") == 0 )
		allocation_failures();
	if( strcmp(group, "all") == 0 || strcmp(group, "printer") == 0 )
		printer_values();
	if( strcmp(group, "all") == 0 || strncmp(group, "long-", 5) == 0 ) {
		pthread_t thread;
		pthread_attr_t attributes;
		char exec_group[] = "long-exec", print_group[] = "long-print";
		char *long_groups[] = { exec_group, print_group };
		size_t index;
		check( pthread_attr_init(&attributes) == 0 );
		/* Match a small worker stack while keeping input buffers below 1 MiB. */
		check( pthread_attr_setstacksize(&attributes, 64 * 1024) == 0 );
		for( index = 0; index < 2; index++ ) {
			if( strcmp(group, "all") == 0 || strcmp(group, long_groups[index]) == 0 ) {
				check( pthread_create(&thread, &attributes, long_call, long_groups[index]) == 0 );
				check( pthread_join(thread, NULL) == 0 );
			}
		}
		check( pthread_attr_destroy(&attributes) == 0 );
	}
	check( outstanding == 0 );
	printf( "UNIX_TRANSIENT_BUFFERS_PASS checks=%u\n", checks );
	return 0;
}

/* end of unix-transient-buffers.c */
