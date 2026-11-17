/*
 * Project: FreeBASIC Linux runtime tests
 * File: linux-keyboard-values.c
 * Purpose: Qualify raw keyboard bytes, failed ioctl results and X11 key bits.
 * Responsibilities: Observe production handlers with bounded native callbacks.
 * Real terminal mode ownership belongs to the console PTY regressions.
 */
#include "drivers/linux/io_multikey.c"
#include "console/scancodes.c"
#include <stddef.h>

unsigned char fb_x11keycode_to_scancode[256];
FBCONSOLE fb_console_context;
static unsigned long checks;
static unsigned char incoming[128], query_bits[32];
static ssize_t incoming_size;
static int fail_mapping, ioctls, events, observed_key, focused = TRUE;
static max_align_t display_token;

ssize_t __wrap_read( int fd, void *buffer, size_t size );
int __wrap_ioctl( int fd, unsigned long operation, ... );
int __wrap_kill( pid_t process, int signal_number );

static void check( int condition )
{
	checks++;
	if( !condition ) {
		fprintf( stderr, "LINUX_KEYBOARD_VALUES_FAIL check=%lu\n", checks );
		exit(1);
	}
}

ssize_t __wrap_read( int fd, void *buffer, size_t size )
{
	check( fd == 7 && size == sizeof(incoming) );
	if( incoming_size > 0 )
		memcpy( buffer, incoming, (size_t)incoming_size );
	return incoming_size;
}

int __wrap_ioctl( int fd, unsigned long operation, ... )
{
	va_list args;
	struct kbentry *entry;
	check( fd == 7 && operation == KDGKBENT );
	va_start( args, operation );
	entry = va_arg( args, struct kbentry * );
	va_end( args );
	check( entry->kb_table <= 15 && entry->kb_index < 128 );
	entry->kb_value = K(KT_LATIN, 'a');
	ioctls++;
	return fail_mapping ? -1 : 0;
}

int __wrap_kill( pid_t process, int signal_number )
{
	(void)process;
	(void)signal_number;
	check( FALSE );
	return -1;
}

int fb_hXTermHasFocus( void )
{
	return focused;
}

static void key_event( int pressed, int repeated, int scan, int key )
{
	check( pressed == 0 || pressed == 128 );
	check( repeated == 0 || repeated == 1 );
	check( scan >= 0 && scan < 128 );
	observed_key = key;
	events++;
}

#ifdef FB_TEST_ORIGINAL
static void query_map( Display *connection, unsigned char *bits )
#else
static int query_map( Display *connection, char *bits )
#endif
{
	check( connection == (Display *)&display_token );
	memcpy( bits, query_bits, sizeof(query_bits) );
#ifndef FB_TEST_ORIGINAL
	return 1;
#endif
}

static void reset_console( void )
{
	memset( key_state, 0, sizeof(key_state) );
	key_head = key_tail = 0;
	key_leds = ioctls = events = observed_key = 0;
	key_fd = 7;
	gfx_key_handler = key_event;
}

int main( int argc, char **argv )
{
	check( argc == 2 );
	if( strcmp( argv[1], "ioctl-failure" ) == 0 ) {
		reset_console();
		incoming[0] = SC_A;
		incoming_size = 1;
		fail_mapping = TRUE;
		keyboard_console_handler();
		check( ioctls == 1 && events == 1 );
		check( observed_key == 0 && key_head == key_tail );
	} else if( strcmp( argv[1], "console-bytes" ) == 0 ) {
		for( unsigned int byte = 0; byte < 256; byte++ ) {
			unsigned char scan = kernel_to_scancode[byte & 127u];
			int expected = fb_hScancodeToExtendedKey(scan);
			check( expected >= 0 && expected <= 65535 );
			if( expected == 0 )
				expected = scan == SC_BACKSPACE ? 8 : 'a';
			reset_console();
			incoming[0] = (unsigned char)byte;
			incoming_size = 1;
			keyboard_console_handler();
			check( events == 1 && ioctls == 1 && observed_key == expected );
			check( key_state[scan] == ((byte & 128u) ? 0 : 128) );
			check( key_tail == ((byte & 128u) ? 0 : 1) );
			if( key_tail )
				check( key_buffer[0] == expected );
		}
		for( ssize_t size = -1; size <= 0; size++ ) {
			reset_console();
			incoming_size = size;
			keyboard_console_handler();
			check( events == 0 && ioctls == 0 );
		}
		reset_console();
		memset( incoming, SC_A, sizeof(incoming) );
		incoming_size = (ssize_t)sizeof(incoming);
		keyboard_console_handler();
		check( events == 128 && ioctls == 128 );
	} else {
		check( strcmp( argv[1], "X11-bits" ) == 0 );
		display = (Display *)&display_token;
		X.QueryKeymap = query_map;
		for( unsigned int i = 0; i < 256; i++ )
			fb_x11keycode_to_scancode[i] = (unsigned char)(i % 128u);
		for( unsigned int bit = 0; bit < 256; bit++ ) {
			memset( query_bits, 0, sizeof(query_bits) );
			query_bits[bit / 8u] = (unsigned char)(1u << (bit % 8u));
			keyboard_x11_handler();
			for( unsigned int scan = 0; scan < 128; scan++ )
				check( key_state[scan] == (scan == bit % 128u) );
		}
		focused = FALSE;
		memset( key_state, 0x55, sizeof(key_state) );
		keyboard_x11_handler();
		for( size_t i = 0; i < sizeof(key_state); i++ )
			check( key_state[i] == 0x55 );
	}
	fprintf( stderr, "LINUX_KEYBOARD_VALUES_PASS checks=%lu\n", checks );
	return 0;
}

/* end of linux-keyboard-values.c */
