/*
    FreeBASIC Windows CE Exampleageddon guest runner
    -------------------------------------------------

    File: tests/wince/exampleageddon-runner.c

    Purpose:

        Run one bounded campaign of executables inside Windows CE.

    Responsibilities:

        - read executable definitions from the CERF shared folder
        - launch each staged executable with its requested arguments
        - terminate examples that exceed the per-process time limit
        - publish every exit status before writing the completion marker
        - preserve enough Windows CE program memory for child processes

    This file intentionally does NOT contain:

        - example discovery or classification
        - target compilation or resource staging
        - emulator startup and shutdown
        - interactive, graphics, audio, or network automation
        - host-side campaign construction
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define OBJECT_STORE_FLOOR_BYTES (16UL * 1024UL * 1024UL)
#define DEFAULT_TIMEOUT_MS 15000UL
#define MAX_TIMEOUT_SECONDS 3600UL
#define MAX_CASE_ID 64
#define MAX_PROGRAM_PATH 260
#define MAX_ARGUMENTS 512
#define MAX_COMMAND_LINE 1024
#define MAX_MANIFEST_LINE 1024

/* CeGCC omits these coredll declarations even though Windows CE exports them. */
WINBASEAPI BOOL WINAPI GetSystemMemoryDivision( LPDWORD store_pages,
	                                             LPDWORD ram_pages,
	                                             LPDWORD page_size );
WINBASEAPI DWORD WINAPI SetSystemMemoryDivision( DWORD store_pages );

struct case_definition {
	char case_id[MAX_CASE_ID];
	char program_path[MAX_PROGRAM_PATH];
	char arguments[MAX_ARGUMENTS];
	DWORD timeout_ms;
};

static const wchar_t manifest_path[] =
	L"\\Storage Card\\exampleageddon-manifest.txt";
static const wchar_t result_path[] =
	L"\\Storage Card\\exampleageddon.result";
static const wchar_t completion_path[] =
	L"\\Storage Card\\exampleageddon.done";
static const wchar_t journal_path[] =
	L"\\Storage Card\\exampleageddon.journal";
static const wchar_t memory_path[] =
	L"\\Storage Card\\exampleageddon-memory.txt";

static int widen_ascii( const char *source, wchar_t *destination,
	                    size_t destination_chars )
{
	size_t index;
	size_t length;

	if( source == NULL || destination == NULL || destination_chars == 0 )
		return 0;

	length = strlen( source );
	if( length >= destination_chars )
		return 0;

	for( index = 0; index <= length; ++index )
		destination[index] = (unsigned char)source[index];

	return 1;
}

static int copy_text( char *destination, size_t destination_bytes,
	                  const char *source )
{
	size_t length;

	if( destination == NULL || destination_bytes == 0 || source == NULL )
		return 0;

	length = strlen( source );
	if( length >= destination_bytes )
		return 0;

	memcpy( destination, source, length + 1 );
	return 1;
}

static int read_text_line( HANDLE file, char *line, size_t line_bytes )
{
	char character;
	DWORD bytes_read;
	int overflow = 0;
	size_t length = 0;

	if( file == INVALID_HANDLE_VALUE || line == NULL || line_bytes == 0 )
		return -2;

	for( ;; ) {
		if( !ReadFile( file, &character, 1, &bytes_read, NULL ) )
			return -2;

		if( bytes_read == 0 ) {
			if( length == 0 && !overflow )
				return 0;
			break;
		}

		if( character == '\n' )
			break;
		if( character == '\r' )
			continue;

		if( length + 1 < line_bytes ) {
			line[length] = character;
			length += 1;
		} else {
			overflow = 1;
		}
	}

	line[length] = '\0';
	return overflow ? -1 : 1;
}

static int write_text( HANDLE file, const char *text )
{
	DWORD bytes_written;
	size_t length;

	if( file == INVALID_HANDLE_VALUE || text == NULL )
		return 0;

	length = strlen( text );
	if( length > 0xffffffffUL )
		return 0;

	if( !WriteFile( file, text, (DWORD)length, &bytes_written, NULL ) ||
	    bytes_written != (DWORD)length )
		return 0;

	return FlushFileBuffers( file ) != 0;
}

static int write_case_result( HANDLE results, const char *case_id,
	                          DWORD exit_code )
{
	char result_line[MAX_CASE_ID + 32];
	int written;

	written = snprintf( result_line, sizeof( result_line ),
	                    "%s\t%lu\r\n", case_id,
	                    (unsigned long)exit_code );
	if( written < 0 || (size_t)written >= sizeof( result_line ) )
		return 0;

	return write_text( results, result_line );
}

static int write_journal( HANDLE journal, const char *event,
	                      const char *case_id, DWORD value )
{
	char journal_line[MAX_CASE_ID + 64];
	int written;

	written = snprintf( journal_line, sizeof( journal_line ),
	                    "%s\t%s\t%lu\r\n", event, case_id,
	                    (unsigned long)value );
	if( written < 0 || (size_t)written >= sizeof( journal_line ) )
		return 0;

	return write_text( journal, journal_line );
}

static int valid_case_id( const char *case_id )
{
	size_t index;
	size_t length;

	if( case_id == NULL )
		return 0;

	length = strlen( case_id );
	if( length == 0 || length >= MAX_CASE_ID )
		return 0;

	for( index = 0; index < length; ++index ) {
		unsigned char character = (unsigned char)case_id[index];

		if( (character >= 'a' && character <= 'z') ||
		    (character >= 'A' && character <= 'Z') ||
		    (character >= '0' && character <= '9') ||
		    character == '-' || character == '_' || character == '.' )
			continue;

		return 0;
	}

	return 1;
}

static int valid_relative_path( const char *path )
{
	size_t index;
	size_t length;

	if( path == NULL )
		return 0;

	length = strlen( path );
	if( length == 0 || length + sizeof( "\\Storage Card\\" ) >
	                   MAX_PROGRAM_PATH )
		return 0;

	if( path[0] == '\\' || path[0] == '/' || strstr( path, ".." ) != NULL )
		return 0;

	for( index = 0; index < length; ++index ) {
		unsigned char character = (unsigned char)path[index];

		if( character < 32 || character == ':' || character == '*' ||
		    character == '?' || character == '"' || character == '<' ||
		    character == '>' || character == '|' )
			return 0;
	}

	return 1;
}

static int valid_arguments( const char *arguments )
{
	size_t index;
	size_t length;

	if( arguments == NULL )
		return 0;

	length = strlen( arguments );
	if( length >= MAX_ARGUMENTS )
		return 0;

	for( index = 0; index < length; ++index ) {
		if( (unsigned char)arguments[index] < 32 )
			return 0;
	}

	return 1;
}

static int parse_timeout( const char *text, DWORD *timeout_ms )
{
	char *end;
	unsigned long seconds;

	if( text == NULL || timeout_ms == NULL || text[0] == '\0' )
		return 0;

	seconds = strtoul( text, &end, 10 );
	if( *end != '\0' || seconds == 0 || seconds > MAX_TIMEOUT_SECONDS )
		return 0;

	*timeout_ms = (DWORD)(seconds * 1000UL);
	return 1;
}

static int build_program_path( const char *relative_path,
	                           char *program_path,
	                           size_t program_path_bytes )
{
	int written;
	size_t index;

	if( !valid_relative_path( relative_path ) )
		return 0;

	written = snprintf( program_path, program_path_bytes,
	                    "\\Storage Card\\%s", relative_path );
	if( written < 0 || (size_t)written >= program_path_bytes )
		return 0;

	for( index = 0; program_path[index] != '\0'; ++index ) {
		if( program_path[index] == '/' )
			program_path[index] = '\\';
	}

	return 1;
}

static int parse_case_definition( char *line,
	                              struct case_definition *definition )
{
	char default_relative_path[MAX_PROGRAM_PATH];
	char *arguments;
	char *program;
	char *separator;
	char *timeout;
	size_t line_length;
	int written;

	if( line == NULL || definition == NULL )
		return -1;

	line_length = strcspn( line, "\r\n" );
	line[line_length] = '\0';

	if( line[0] == '\0' || line[0] == '#' )
		return 0;

	memset( definition, 0, sizeof( *definition ) );
	definition->timeout_ms = DEFAULT_TIMEOUT_MS;

	separator = strchr( line, '\t' );
	if( separator == NULL ) {
		if( !valid_case_id( line ) ||
		    !copy_text( definition->case_id,
		                sizeof( definition->case_id ), line ) )
			return -1;

		written = snprintf( default_relative_path,
		                    sizeof( default_relative_path ),
		                    "exampleageddon\\%s\\runner.exe", line );
		if( written < 0 ||
		    (size_t)written >= sizeof( default_relative_path ) )
			return -1;

		if( !build_program_path( default_relative_path,
		                         definition->program_path,
		                         sizeof( definition->program_path ) ) )
			return -1;

		return 1;
	}

	*separator = '\0';
	program = separator + 1;
	separator = strchr( program, '\t' );
	if( separator == NULL )
		return -1;

	*separator = '\0';
	timeout = separator + 1;
	separator = strchr( timeout, '\t' );
	if( separator == NULL )
		return -1;

	*separator = '\0';
	arguments = separator + 1;
	if( strchr( arguments, '\t' ) != NULL )
		return -1;

	if( !valid_case_id( line ) || !valid_arguments( arguments ) ||
	    !copy_text( definition->case_id,
	                sizeof( definition->case_id ), line ) ||
	    !copy_text( definition->arguments,
	                sizeof( definition->arguments ), arguments ) ||
	    !parse_timeout( timeout, &definition->timeout_ms ) ||
	    !build_program_path( program, definition->program_path,
	                         sizeof( definition->program_path ) ) )
		return -1;

	return 1;
}

static DWORD run_case( const struct case_definition *definition,
	                   HANDLE journal )
{
	char command_line[MAX_COMMAND_LINE];
	wchar_t wide_command_line[MAX_COMMAND_LINE];
	wchar_t wide_program_path[MAX_PROGRAM_PATH];
	PROCESS_INFORMATION process_info;
	STARTUPINFOW startup_info;
	DWORD exit_code;
	DWORD wait_result;
	int written;

	if( definition == NULL )
		return ERROR_INVALID_PARAMETER;

	/*
	    lpApplicationName already identifies the executable.  The MIPS CRT
	    obtains argv[0] from GetModuleFileNameW, so this buffer must contain
	    only the argument tail or Command(1) would repeat the program path.
	*/
	written = snprintf( command_line, sizeof( command_line ),
	                    "%s", definition->arguments );

	if( written < 0 || (size_t)written >= sizeof( command_line ) )
		return ERROR_BUFFER_OVERFLOW;

	if( !widen_ascii( definition->program_path, wide_program_path,
	                  sizeof( wide_program_path ) /
	                  sizeof( wide_program_path[0] ) ) ||
	    !widen_ascii( command_line, wide_command_line,
	                  sizeof( wide_command_line ) /
	                  sizeof( wide_command_line[0] ) ) )
		return ERROR_BUFFER_OVERFLOW;

	memset( &startup_info, 0, sizeof( startup_info ) );
	memset( &process_info, 0, sizeof( process_info ) );
	startup_info.cb = sizeof( startup_info );

	if( !CreateProcessW( wide_program_path, wide_command_line,
	                     NULL, NULL, FALSE, 0,
	                     NULL, NULL, &startup_info, &process_info ) )
		return GetLastError();

	write_journal( journal, "CREATED", definition->case_id,
	               process_info.dwProcessId );

	wait_result = WaitForSingleObject( process_info.hProcess,
	                                   definition->timeout_ms );
	if( wait_result == WAIT_TIMEOUT ) {
		write_journal( journal, "TIMEOUT", definition->case_id,
		               definition->timeout_ms );
		TerminateProcess( process_info.hProcess, 124 );
		WaitForSingleObject( process_info.hProcess, 2000 );
		exit_code = 124;
	} else if( wait_result != WAIT_OBJECT_0 ) {
		exit_code = GetLastError();
	} else if( !GetExitCodeProcess( process_info.hProcess, &exit_code ) ) {
		exit_code = GetLastError();
	}

	CloseHandle( process_info.hThread );
	CloseHandle( process_info.hProcess );
	return exit_code;
}

/* ------------------------------------------------------------------------- */
/* Windows CE memory division                                                */
/* ------------------------------------------------------------------------- */

static void rebalance_program_memory( void )
{
	MEMORYSTATUS before;
	MEMORYSTATUS after;
	DWORD store_pages = 0;
	DWORD ram_pages = 0;
	DWORD page_size = 0;
	DWORD original_store_pages = 0;
	DWORD target_store_pages = 0;
	DWORD division_result = 0xffffffffUL;
	BOOL division_available;
	HANDLE file;
	char line[256];
	int written;

	before.dwLength = sizeof( before );
	GlobalMemoryStatus( &before );

	division_available = GetSystemMemoryDivision( &store_pages,
	                                              &ram_pages,
	                                              &page_size );
	original_store_pages = store_pages;

	if( division_available && page_size > 0 ) {
		target_store_pages = OBJECT_STORE_FLOOR_BYTES / page_size;
		if( (OBJECT_STORE_FLOOR_BYTES % page_size) != 0 )
			target_store_pages += 1;

		/*
		    Campaign files live on the external Storage Card.  Keep a useful
		    16 MiB object store, but return any excess to program memory so CE
		    can create test processes and their worker threads.
		*/
		if( target_store_pages > 0 && store_pages > target_store_pages )
			division_result = SetSystemMemoryDivision( target_store_pages );
	}

	store_pages = 0;
	ram_pages = 0;
	page_size = 0;
	GetSystemMemoryDivision( &store_pages, &ram_pages, &page_size );

	after.dwLength = sizeof( after );
	GlobalMemoryStatus( &after );

	file = CreateFileW( memory_path, GENERIC_WRITE, FILE_SHARE_READ, NULL,
	                    CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if( file == INVALID_HANDLE_VALUE )
		return;

	written = snprintf( line, sizeof( line ),
	                    "store-pages-before=%lu\r\n"
	                    "store-pages-after=%lu\r\n"
	                    "ram-pages-after=%lu\r\n"
	                    "page-size=%lu\r\n"
	                    "division-result=%lu\r\n"
	                    "available-before=%lu\r\n"
	                    "available-after=%lu\r\n",
	                    (unsigned long)original_store_pages,
	                    (unsigned long)store_pages,
	                    (unsigned long)ram_pages,
	                    (unsigned long)page_size,
	                    (unsigned long)division_result,
	                    (unsigned long)before.dwAvailPhys,
	                    (unsigned long)after.dwAvailPhys );

	if( written > 0 && (size_t)written < sizeof( line ) )
		write_text( file, line );

	CloseHandle( file );
}

int main( void )
{
	rebalance_program_memory();
	char completion_line[64];
	char line[MAX_MANIFEST_LINE];
	struct case_definition definition;
	HANDLE completion;
	HANDLE journal;
	HANDLE manifest;
	HANDLE results;
	unsigned long case_count = 0;
	unsigned long invalid_count = 0;
	int completion_length;
	int line_result;

	results = CreateFileW( result_path, GENERIC_WRITE,
	                       FILE_SHARE_READ | FILE_SHARE_WRITE,
	                       NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if( results == INVALID_HANDLE_VALUE )
		return 3;

	journal = CreateFileW( journal_path, GENERIC_WRITE,
	                       FILE_SHARE_READ | FILE_SHARE_WRITE,
	                       NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if( journal == INVALID_HANDLE_VALUE ) {
		CloseHandle( results );
		return 5;
	}
	write_journal( journal, "MAIN", "campaign", 0 );

	manifest = CreateFileW( manifest_path, GENERIC_READ, FILE_SHARE_READ,
	                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if( manifest == INVALID_HANDLE_VALUE ) {
		write_case_result( results, "campaign-manifest", GetLastError() );
		CloseHandle( journal );
		CloseHandle( results );
		return 2;
	}

	while( (line_result = read_text_line( manifest, line,
	                                     sizeof( line ))) != 0 ) {
		DWORD exit_code;
		int parse_result;

		if( line_result == -2 ) {
			write_case_result( results, "campaign-read", GetLastError() );
			invalid_count += 1;
			break;
		}

		if( line_result == -1 ) {
			invalid_count += 1;
			write_case_result( results, "campaign-line-too-long",
			                   ERROR_BUFFER_OVERFLOW );
			continue;
		}

		parse_result = parse_case_definition( line, &definition );
		if( parse_result == 0 )
			continue;

		if( parse_result < 0 ) {
			invalid_count += 1;
			write_case_result( results, "campaign-invalid",
			                   ERROR_INVALID_DATA );
			continue;
		}

		write_journal( journal, "START", definition.case_id,
		               definition.timeout_ms );
		exit_code = run_case( &definition, journal );
		write_journal( journal, "DONE", definition.case_id, exit_code );
		if( !write_case_result( results, definition.case_id, exit_code ) ) {
			invalid_count += 1;
			break;
		}
		case_count += 1;
	}

	CloseHandle( results );
	CloseHandle( journal );
	CloseHandle( manifest );

	completion = CreateFileW( completion_path, GENERIC_WRITE,
	                          FILE_SHARE_READ | FILE_SHARE_WRITE,
	                          NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if( completion == INVALID_HANDLE_VALUE )
		return 4;

	completion_length = snprintf( completion_line, sizeof( completion_line ),
	                              "%lu\t%lu\r\n", case_count,
	                              invalid_count );
	if( completion_length < 0 ||
	    (size_t)completion_length >= sizeof( completion_line ) ||
	    !write_text( completion, completion_line ) ) {
		CloseHandle( completion );
		return 4;
	}

	CloseHandle( completion );
	return 0;
}

/* end of tests/wince/exampleageddon-runner.c */
