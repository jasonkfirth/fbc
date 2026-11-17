/*
 * Project: FreeBASIC Unix runtime tests
 * File: termcap-key-nodes.c
 * Purpose: Verify the native key lookup tree and its reserved storage.
 * Responsibilities: Check actual table codes, allocation failure and ownership.
 * Physical terminal database behavior belongs to the console PTY tests.
 */
#include "drivers/unix/io_inkey.c"

FBCONSOLE fb_console_context;
static unsigned long checks;
static size_t allocated, released;
static int fail_allocation, input[8], input_count, input_offset, mouse_events;

void *__real_malloc( size_t size );
void *__real_calloc( size_t count, size_t size );
void __real_free( void *pointer );
void *__wrap_malloc( size_t size );
void *__wrap_calloc( size_t count, size_t size );
void __wrap_free( void *pointer );
char *__wrap_tgetstr( const char *id, char **area );

static void check( int condition )
{
	checks++;
	if( !condition ) {
		fprintf( stderr, "TERMCAP_KEY_NODES_FAIL check=%lu\n", checks );
		exit(1);
	}
}

void *__wrap_malloc( size_t size )
{
	void *pointer;
	check( size == sizeof(NODE) );
	if( fail_allocation )
		return NULL;
	pointer = __real_malloc(size);
	check( pointer != NULL );
	allocated++;
	return pointer;
}

void *__wrap_calloc( size_t count, size_t size )
{
	void *pointer;
	check( count == 1 && size == sizeof(NODE) );
	if( fail_allocation )
		return NULL;
	pointer = __real_calloc(count, size);
	check( pointer != NULL );
	allocated++;
	return pointer;
}

void __wrap_free( void *pointer )
{
	check( pointer != NULL );
	released++;
	__real_free(pointer);
}

char *__wrap_tgetstr( const char *id, char **area )
{
	(void)id;
	(void)area;
	check(FALSE);
	return NULL;
}

static int next_key( void )
{
	check( input_offset < input_count );
	return input[input_offset++];
}

static void mouse_update( int button, int column, int row )
{
	check( button == 12 && column == 34 && row == 56 );
	mouse_events++;
}

static void release_nodes( NODE *node )
{
	if( node == NULL )
		return;
#ifndef FB_TEST_ORIGINAL
	check( node->key_alignment_padding == 0 );
#if defined(__SIZEOF_POINTER__) && (__SIZEOF_POINTER__ == 8)
	for( size_t i = 0; i < sizeof(node->pointer_alignment_padding); i++ )
		check( node->pointer_alignment_padding[i] == 0 );
#endif
#endif
	release_nodes(node->next);
	release_nodes(node->child);
	free(node);
}

int main( void )
{
	NODE *saved;
	for( const KEY_DATA *data = key_data; data->cap != NULL; data++ )
		add_key(&root_node, data->cap, data->code);
	add_key(&root_node, "[M", KEY_MOUSE);
	check( root_node != NULL );
	fb_console_context.keyboard_getch = next_key;
	fb_console_context.mouse_update = mouse_update;
	for( const KEY_DATA *data = key_data; data->cap != NULL; data++ ) {
		check( strlen(data->cap) == 2 && data->code > 0 );
#ifndef FB_TEST_ORIGINAL
		for( size_t i = 0; i < sizeof(data->tail_padding); i++ )
			check( data->tail_padding[i] == 0 );
#endif
		input[0] = 27;
		input[1] = (unsigned char)data->cap[0];
		input[2] = (unsigned char)data->cap[1];
		input_count = 3;
		input_offset = 0;
		check( get_input() == data->code && input_offset == input_count );
	}
	input[0] = 27;
	input[1] = '[';
	input[2] = 'M';
	input[3] = 12;
	input[4] = 34;
	input[5] = 56;
	input_count = 6;
	input_offset = 0;
	check( get_input() == -1 && mouse_events == 1 && input_offset == input_count );
	saved = root_node;
	fail_allocation = TRUE;
	add_key(&root_node, "zz", 123);
	check( root_node == saved );
	release_nodes(root_node);
	root_node = NULL;
	check( allocated == released );
	fprintf( stderr, "TERMCAP_KEY_NODES_PASS checks=%lu nodes=%zu\n", checks, allocated );
	return 0;
}

/* end of termcap-key-nodes.c */
