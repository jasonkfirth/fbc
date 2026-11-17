/*
 * Project: FreeBASIC runtime tests
 * File: unix-console-bounds.c
 * Purpose: Check Unix console spans, counted output and resize ownership.
 * Responsibilities: Observe production entry points with deterministic OS and
 *     input callbacks, including allocation refusal and background exclusion.
 * Actual terminal interaction is covered by the linked public-API control.
 */
#define _GNU_SOURCE 1
#include "fb.h"
#include "drivers/unix/fb_private_console.h"
#include <limits.h>
#include <locale.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <sys/ioctl.h>

FBCONSOLE fb_console_context;
volatile sig_atomic_t fb_test_console_resized;

int fb_test_console_ioctl( int fd, unsigned long request, ... );
int fb_hTermQuery( int code, int *first, int *second );
void *__wrap_calloc( size_t count, size_t size );
void *__real_calloc( size_t count, size_t size );
void *__wrap_malloc( size_t size );
void *__real_malloc( size_t size );

static pthread_mutex_t console_mutex = PTHREAD_MUTEX_INITIALIZER;
static _Thread_local int lock_depth;
static atomic_ulong checks;
static int query_rows = 3, query_cols = 8, ioctl_fails, fail_allocation;
static int view_first = 1, view_last = 3, terminal_calls;
static int locate_row, locate_col, locate_calls;
static int input_cols = 8, input_x = 3, input_y = 7;
static ssize_t input_length, accepted_read_count;
static int validate_input_count = TRUE;
static atomic_int request_background, background_attempted;
static atomic_int background_test, background_acquired;
static pthread_t main_thread;
static FILE *capture;
static int saved_stdout;

static void check( int condition )
{
	unsigned long current = atomic_fetch_add(&checks, 1) + 1;
	if( !condition ) {
		fprintf(stderr, "UNIX_CONSOLE_BOUNDS_FAIL check=%lu\n", current);
		exit(1);
	}
}

void *__wrap_calloc( size_t count, size_t size )
{
	/* Keep each test allocation within one MiB, including original controls. */
	if( fail_allocation || (count != 0 && size > 1048576u / count) )
		return NULL;
	return __real_calloc(count, size);
}

void *__wrap_malloc( size_t size )
{
	if( fail_allocation || size > 1048576u )
		return NULL;
	return __real_malloc(size);
}

void fb_BgLock( void )
{
	check(lock_depth == 0);
	check(pthread_mutex_lock(&console_mutex) == 0);
	lock_depth = 1;
}

void fb_BgUnlock( void )
{
	check(lock_depth == 1);
	lock_depth = 0;
	check(pthread_mutex_unlock(&console_mutex) == 0);
	if( atomic_load(&background_test) && pthread_equal(pthread_self(), main_thread) ) {
		while( !atomic_load(&background_acquired) )
			sched_yield();
	}
}

int fb_test_console_ioctl( int fd, unsigned long request, ... )
{
	va_list args;
	struct winsize *window;
	(void)fd;
	check(lock_depth == 1 && request == TIOCGWINSZ);
	if( atomic_exchange(&request_background, 0) ) {
		while( !atomic_load(&background_attempted) )
			sched_yield();
	}
	if( ioctl_fails )
		return -1;
	va_start(args, request);
	window = va_arg(args, struct winsize *);
	va_end(args);
	window->ws_row = (unsigned short)query_rows;
	window->ws_col = (unsigned short)query_cols;
	return 0;
}

int fb_hTermQuery( int code, int *first, int *second )
{
	check(lock_depth == 1 && code == SEQ_QUERY_WINDOW);
	*first = query_rows;
	*second = query_cols;
	return TRUE;
}

void fb_hRecheckCursorPos( void ) { check(lock_depth == 1); }
void fb_DevScrnMaybeUpdateWidth( void ) { check(lock_depth == 1); }

int fb_hTermOut( int code, int first, int second )
{
	(void)first;
	(void)second;
	check(lock_depth == 1 && (code == SEQ_LOCATE || code == SEQ_CLEOL));
	terminal_calls++;
	return TRUE;
}

void fb_ConsoleGetView( int *first, int *last )
{
	check(lock_depth == 0);
	*first = view_first;
	*last = view_last;
}

int fb_ConsoleGetMaxRow( void )
{
	check(lock_depth == 0);
	return fb_console_context.h;
}

/* Line-reader callbacks retain the count without allocating a synthetic
   multi-gigabyte line. Assignment callbacks observe its descriptor only. */
void fb_Lock( void ) { }
void fb_Unlock( void ) { }
void fb_PrintBufferEx( const void *buffer, size_t len, int mask )
{ (void)buffer; (void)len; (void)mask; }
void fb_PrintString( int file, FBSTRING_ACCESS *text, int mask )
{ (void)file; (void)text; (void)mask; }
void fb_PrintFixString( int file, const char *text, int mask )
{ (void)file; (void)text; (void)mask; }
void fb_PrintWstr( int file, const FB_WCHAR *text, int mask )
{ (void)file; (void)text; (void)mask; }
int fb_hStrDelTemp( FBSTRING_ACCESS *text ) { (void)text; return 0; }
void fb_StrDelete( FBSTRING_ACCESS *text ) { (void)text; }
void *fb_StrAssign( void *dst, ssize_t dst_size, void *src, ssize_t src_size, int fill )
{ (void)dst_size; (void)src; (void)src_size; (void)fill; return dst; }
FB_WCHAR *fb_WstrAssignFromA( FB_WCHAR *dst, ssize_t dst_chars, void *src, ssize_t src_chars )
{ (void)dst_chars; (void)src; (void)src_chars; return dst; }
char *fb_ReadString( char *buffer, ssize_t len, FILE *file )
{ (void)file; accepted_read_count = len; return buffer; }

int fb_DevFileReadLineDumb( FILE *file, FBSTRING_ACCESS *text, fb_FnDevReadString reader )
{
	char data[2] = { 0, 0 };
	accepted_read_count = -1;
	if( validate_input_count ) {
		check(reader(data, (size_t)FB_STRSIZEMSK + 1, file) == NULL);
		check(accepted_read_count == -1);
	}
	check(reader(data, 1, file) == data && accepted_read_count == 1);
	text->len = input_length;
	return 0;
}

void fb_GetXY( int *col, int *row )
{ if( col ) *col = input_x; if( row ) *row = input_y; }
void fb_GetSize( int *cols, int *rows )
{ if( cols ) *cols = input_cols; if( rows ) *rows = 25; }
int fb_Locate( int row, int col, int cursor, int start, int stop )
{
	check(cursor == -1 && start == 0 && stop == 0);
	locate_row = row;
	locate_col = col;
	locate_calls++;
	return 0;
}

static void reset_console( int width, int height )
{
	size_t cells;
	free(fb_console_context.char_buffer);
	memset(&fb_console_context, 0, sizeof(fb_console_context));
	check(fb_hConsoleCellCount(width, height, &cells));
	fb_console_context.char_buffer = calloc(2, cells);
	check(fb_console_context.char_buffer != NULL);
	fb_console_context.attr_buffer = fb_console_context.char_buffer + cells;
	fb_console_context.w = width;
	fb_console_context.h = height;
	fb_console_context.cur_x = fb_console_context.cur_y = 1;
	fb_console_context.fg_color = 7;
	fb_console_context.inited = TRUE;
	fb_test_console_resized = FALSE;
}

static void reset_capture( void )
{
	check(fflush(stdout) == 0);
	check(ftruncate(fileno(capture), 0) == 0);
	check(fseek(capture, 0, SEEK_SET) == 0);
}

static void check_output( const unsigned char *expected, size_t length )
{
	unsigned char actual[64];
	check(length <= sizeof(actual));
	check(fflush(stdout) == 0 && fseek(capture, 0, SEEK_SET) == 0);
	if( length != 0 )
		check(fread(actual, 1, length, capture) == length && memcmp(actual, expected, length) == 0);
	check(fgetc(capture) == EOF);
}

static void test_spans( void )
{
	size_t cells, offset;
	int x, y;
	check(!fb_hConsoleCellCount(0, 1, &cells) && cells == 0);
	check(!fb_hConsoleCellCount(-1, 1, &cells));
	check(!fb_hConsoleCellCount(INT_MAX, 2, &cells));
	check(fb_hConsoleCellCount(INT_MAX, 1, &cells) && cells == (size_t)INT_MAX);
	reset_console(8, 3);
	for( y = 1; y <= 3; y++ ) {
		for( x = 1; x <= 8; x++ ) {
			fb_console_context.cur_x = x;
			fb_console_context.cur_y = y;
			check(fb_hConsoleBufferSpan(&offset) == 24 - ((size_t)(y - 1) * 8 + (size_t)(x - 1)));
			check(offset == (size_t)(y - 1) * 8 + (size_t)(x - 1));
		}
	}
	fb_console_context.cur_x = INT_MIN;
	check(fb_hConsoleBufferSpan(&offset) == 0 && offset == 0);
}

static void test_wide_output( void )
{
	const FB_WCHAR embedded[3] = { 'A', 0, 'B' };
	const FB_WCHAR unicode[3] = { 'A', 0xe9, 0x20ac };
	const unsigned char expected[9] = { 27, '%', 'G', 'A', 0, 'B', 27, '%', '@' };
	const unsigned char expected_unicode[12] = { 27, '%', 'G', 'A', 0xc3, 0xa9, 0xe2, 0x82, 0xac, 27, '%', '@' };
	check(setlocale(LC_CTYPE, "C") != NULL);
	reset_console(8, 3);
	reset_capture();
	fb_ConsolePrintBufferWstrEx(embedded, 3, 0);
	check_output(expected, sizeof(expected));
	check(fb_console_context.char_buffer[0] == 'A' && fb_console_context.char_buffer[1] == 0 && fb_console_context.char_buffer[2] == 0);
	check(fb_console_context.cur_x == 4 && fb_console_context.cur_y == 1);
	reset_console(8, 3);
	reset_capture();
	fb_ConsolePrintBufferWstrEx(unicode, 3, 0);
	check_output(expected_unicode, sizeof(expected_unicode));
	check(memcmp(fb_console_context.char_buffer, "A??", 3) == 0);
	check(setlocale(LC_CTYPE, "C.UTF-8") != NULL);
	reset_console(8, 3);
	reset_capture();
	fb_ConsolePrintBufferWstrEx(unicode, 3, 0);
	check_output(expected_unicode, sizeof(expected_unicode));
	check(memcmp(fb_console_context.char_buffer, "A\303\251", 3) == 0);
	reset_capture();
	fb_ConsolePrintBufferWstrEx(NULL, 1, 0);
	fb_ConsolePrintBufferWstrEx(unicode, SIZE_MAX, 0);
	check_output(NULL, 0);
}

static void test_resize( void )
{
	unsigned char *previous;
	reset_console(8, 3);
	memcpy(fb_console_context.char_buffer, "row zero", 8);
	memset(fb_console_context.attr_buffer, 0x37, 24);
	query_cols = 10; query_rows = 4;
	fb_test_console_resized = TRUE;
	fb_BgLock(); fb_hRecheckConsoleSize(FALSE); fb_BgUnlock();
	check(fb_console_context.w == 10 && fb_console_context.h == 4);
	check(memcmp(fb_console_context.char_buffer, "row zero", 8) == 0);
	check(fb_console_context.char_buffer[8] == 0 && fb_console_context.attr_buffer[7] == 0x37);
	previous = fb_console_context.char_buffer;
	query_cols = query_rows = USHRT_MAX;
	fb_test_console_resized = TRUE;
	fb_BgLock(); fb_hRecheckConsoleSize(FALSE); fb_BgUnlock();
	check(fb_console_context.char_buffer == previous && fb_console_context.w == 10);
	query_cols = 9; query_rows = 3; fail_allocation = TRUE;
	fb_test_console_resized = TRUE;
	fb_BgLock(); fb_hRecheckConsoleSize(FALSE); fb_BgUnlock();
	check(fb_console_context.char_buffer == previous && fb_console_context.w == 10);
	fail_allocation = FALSE;
	ioctl_fails = TRUE; query_cols = query_rows = INT_MAX;
	fb_test_console_resized = TRUE;
	fb_BgLock(); fb_hRecheckConsoleSize(FALSE); fb_BgUnlock();
	check(fb_console_context.w == 80 && fb_console_context.h == 25);
	ioctl_fails = FALSE;
}

static void test_clear_and_input( int validate_count )
{
	char destination[4];
	FB_WCHAR wide_destination[4];
	const ssize_t lengths[] = { 0, 1, 7, 8, 9, INT_MAX, FB_STRSIZEMSK };
	size_t i;
	validate_input_count = validate_count;
	input_cols = 8;
	reset_console(8, 3);
	view_first = 0; view_last = INT_MAX; terminal_calls = 0;
	fb_ConsoleClear(2);
	check(terminal_calls == 7 && fb_console_context.cur_x == 1 && fb_console_context.cur_y == 1);
	for( i = 0; i < 24; i++ )
		check(fb_console_context.char_buffer[i] == ' ' && fb_console_context.attr_buffer[i] == 7);
	for( i = 0; i < sizeof(lengths) / sizeof(lengths[0]); i++ ) {
		input_length = lengths[i]; locate_calls = 0;
		check(fb_ConsoleLineInput(NULL, destination, 4, 0, 0, 0) == 0);
		check(fb_ConsoleLineInputWstr(NULL, wide_destination, 4, 0, 0) == 0);
		check(locate_calls == 2 && locate_row == 6);
		check(locate_col == (int)((2u + (size_t)input_length % 8u) % 8u) + 1);
	}
	input_cols = 0; locate_calls = 0;
	check(fb_ConsoleLineInput(NULL, destination, 4, 0, 0, 0) == 0 && locate_calls == 0);
}

static void *background_resize( void *unused )
{
	(void)unused;
	while( atomic_load(&request_background) )
		sched_yield();
	atomic_store(&background_attempted, 1);
	fb_BgLock();
	/* The writer publishes its cells before releasing the resize mutex. */
	check(memcmp(fb_console_context.char_buffer, "TEST", 4) == 0);
	atomic_store(&background_acquired, 1);
	query_cols = 5; query_rows = 2;
	fb_test_console_resized = TRUE;
	fb_hRecheckConsoleSize(FALSE);
	fb_BgUnlock();
	return NULL;
}

static void test_background_resize( int wide )
{
	const FB_WCHAR text[4] = { 'T', 'E', 'S', 'T' };
	pthread_t worker;
	reset_console(8, 3);
	reset_capture();
	query_cols = 8; query_rows = 3;
	atomic_store(&background_acquired, 0);
	atomic_store(&background_attempted, 0);
	atomic_store(&request_background, 1);
	atomic_store(&background_test, 1);
	fb_test_console_resized = TRUE;
	check(pthread_create(&worker, NULL, background_resize, NULL) == 0);
	if( wide )
		fb_ConsolePrintBufferWstrEx(text, 4, 0);
	else
		fb_ConsolePrintBufferEx("TEST", 4, 0);
	check(pthread_join(worker, NULL) == 0);
	atomic_store(&background_test, 0);
	check(fb_console_context.w == 5 && fb_console_context.h == 2);
	check(memcmp(fb_console_context.char_buffer, "TEST", 4) == 0);
}

static void *small_stack_print( void *text )
{
	fb_ConsolePrintBufferWstrEx(text, 16384, 0);
	return NULL;
}

static void test_long_output( void )
{
	FB_WCHAR *text = malloc(16384 * sizeof(*text));
	pthread_attr_t attributes;
	pthread_t worker;
	size_t i;
	check(text != NULL);
	for( i = 0; i < 16384; i++ )
		text[i] = 'A';
	reset_console(8, 3);
	reset_capture();
	fail_allocation = TRUE;
	fb_ConsolePrintBufferWstrEx(text, 16384, 0);
	fail_allocation = FALSE;
	check_output(NULL, 0);
	check(fb_console_context.char_buffer[0] == 0 && fb_console_context.cur_x == 1);
	check(pthread_attr_init(&attributes) == 0);
	/* The former input-sized alloca exceeded this native stack. */
	check(pthread_attr_setstacksize(&attributes, 65536) == 0);
	check(pthread_create(&worker, &attributes, small_stack_print, text) == 0);
	check(pthread_join(worker, NULL) == 0 && pthread_attr_destroy(&attributes) == 0);
	check(fflush(stdout) == 0 && fseek(capture, 0, SEEK_SET) == 0);
	check(fgetc(capture) == 27 && fgetc(capture) == '%' && fgetc(capture) == 'G');
	for( i = 0; i < 16384; i++ )
		check(fgetc(capture) == 'A');
	check(fgetc(capture) == 27 && fgetc(capture) == '%' && fgetc(capture) == '@');
	check(fgetc(capture) == EOF);
	free(text);
}

int main( int argc, char **argv )
{
	const char *which = argc > 1 ? argv[1] : "all";
	int all = strcmp(which, "all") == 0;
	int groups = 0;
	main_thread = pthread_self();
	saved_stdout = dup(STDOUT_FILENO);
	capture = tmpfile();
	check(saved_stdout >= 0 && capture != NULL && dup2(fileno(capture), STDOUT_FILENO) >= 0);
	if( all || strcmp(which, "spans") == 0 ) { test_spans(); groups++; }
	if( all || strcmp(which, "wide") == 0 ) { test_wide_output(); groups++; }
	if( all || strcmp(which, "resize") == 0 ) { test_resize(); groups++; }
	if( all || strcmp(which, "input") == 0 ) { test_clear_and_input(TRUE); groups++; }
	if( all || strcmp(which, "input-cursor") == 0 ) { test_clear_and_input(FALSE); groups++; }
	if( all || strcmp(which, "background-narrow") == 0 ) { test_background_resize(FALSE); groups++; }
	if( all || strcmp(which, "background-wide") == 0 ) { test_background_resize(TRUE); groups++; }
	if( all || strcmp(which, "long") == 0 ) { test_long_output(); groups++; }
	check(fflush(stdout) == 0 && dup2(saved_stdout, STDOUT_FILENO) >= 0);
	check(close(saved_stdout) == 0 && fclose(capture) == 0);
	free(fb_console_context.char_buffer);
	if( groups == 0 )
		return 2;
	printf("UNIX_CONSOLE_BOUNDS_PASS checks=%lu\n", atomic_load(&checks));
	return 0;
}

/* end of unix-console-bounds.c */
