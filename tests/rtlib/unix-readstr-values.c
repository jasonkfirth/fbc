/*
 * Project: FreeBASIC Unix runtime tests
 * File: unix-readstr-values.c
 * Purpose: Qualify console line capacities and byte-valued input.
 * Responsibilities: Observe production editing and bounded libc dispatch.
 * Physical terminal behavior belongs to the console PTY regression.
 */
#include "drivers/unix/io_readstr.c"

FBCONSOLE fb_console_context;
static unsigned long checks;
static int keys[16], key_count, key_offset, fgets_calls, capacity;

char *__wrap_fgets( char *buffer, int size, FILE *stream );
int __wrap_fputc( int value, FILE *stream );

static void check( int condition )
{
	checks++;
	if( !condition ) {
		fprintf( stderr, "UNIX_READSTR_VALUES_FAIL check=%lu\n", checks );
		exit(1);
	}
}

char *__wrap_fgets( char *buffer, int size, FILE *stream )
{
	check( stream == stdin );
	fgets_calls++;
	capacity = size;
	if( buffer == NULL || size <= 0 )
		return NULL;
	buffer[0] = '\0';
	if( size > 1 ) {
		buffer[0] = 'r';
		buffer[1] = '\0';
	}
	return buffer;
}

int __wrap_fputc( int value, FILE *stream )
{
	check( value == '\n' && stream == stdout );
	return value;
}

int fb_hGetCh( int remove )
{
	check( remove == TRUE && key_offset < key_count );
	return keys[key_offset++];
}

FBCALL void fb_Delay( int milliseconds )
{
	check( milliseconds == 10 );
}

void fb_ConsoleGetSize( int *columns, int *rows )
{
	*columns = 80;
	check( rows == NULL );
}

void fb_ConsoleGetXY( int *column, int *row )
{
	*column = 2;
	*row = 1;
}

int fb_hTermOut( int code, int first, int second )
{
	(void)first;
	(void)second;
	check( code == SEQ_LOCATE || code == SEQ_DEL_CHAR );
	return TRUE;
}

void fb_ConsolePrintBuffer( const char *text, int mask )
{
	(void)text;
	check( mask == 0 );
}

int main( int argc, char **argv )
{
	char output[8];
	check( argc == 2 );
	if( strcmp( argv[1], "capacity" ) == 0 ) {
		check( fb_ConsoleReadStr( NULL, 8 ) == NULL && fgets_calls == 0 );
		check( fb_ConsoleReadStr( output, 0 ) == NULL && fgets_calls == 0 );
		check( fb_ConsoleReadStr( output, -1 ) == NULL && fgets_calls == 0 );
		check( fb_ConsoleReadStr( output, 1 ) == output && capacity == 1 );
		check( output[0] == '\0' );
		check( fb_ConsoleReadStr( output, 8 ) == output && capacity == 8 );
		if( sizeof(ssize_t) > sizeof(int) ) {
			ssize_t large = INT_MAX;
			large++;
			check( fb_ConsoleReadStr( output, large ) == output && capacity == INT_MAX );
		}
	} else {
		fb_console_context.inited = INIT_CONSOLE;
		if( strcmp( argv[1], "bytes" ) == 0 ) {
			for( int byte = 0; byte < 256; byte++ ) {
				if( byte == '\r' || byte == '\t' || byte == 8 )
					continue;
				memset( output, 0x55, sizeof(output) );
				keys[0] = byte;
				keys[1] = '\r';
				key_count = 2;
				key_offset = 0;
				check( fb_ConsoleReadStr( output, 4 ) == output );
				check( (unsigned char)output[0] == byte && output[1] == '\r' && output[2] == '\0' );
				check( output[3] == 0x55 && key_offset == 2 );
			}
		} else if( strcmp( argv[1], "negative" ) == 0 ) {
			keys[0] = -2;
			keys[1] = 0x100;
			keys[2] = 'a';
			keys[3] = '\r';
			key_count = 4;
			check( fb_ConsoleReadStr( output, 4 ) == output );
			check( output[0] == 'a' && output[1] == '\r' && output[2] == '\0' );
		} else {
			static const int editing[] = { 'a', 'b', 8, 'c', '\t', 'd', '\r' };
			check( strcmp( argv[1], "editing" ) == 0 );
			memcpy( keys, editing, sizeof(editing) );
			key_count = (int)(sizeof(editing) / sizeof(editing[0]));
			check( fb_ConsoleReadStr( output, 4 ) == output );
			check( strcmp( output, "acd" ) == 0 && key_offset == key_count );
		}
	}
	fprintf( stderr, "UNIX_READSTR_VALUES_PASS checks=%lu\n", checks );
	return 0;
}

/* end of unix-readstr-values.c */
