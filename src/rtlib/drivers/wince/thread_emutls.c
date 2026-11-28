/*
    Project: FreeBASIC Runtime Library
    File: drivers/wince/thread_emutls.c
    Purpose: Provide initialized C thread-local storage through Windows CE TLS.
    Responsibilities: Allocate one emulated TLS object per variable and thread.
    This file intentionally does NOT contain: FreeBASIC ThreadLocal parsing or scheduling policy.

    CeGCC's emulated TLS object references are process-shared when the Windows
    CE runtime starts workers with CreateThread.  FreeBASIC uses CreateThread
    so a CRT-specific TLS table is not guaranteed to be initialized.  Keep the
    compiler's emutls object format, but store each object in a CE Tls slot.
*/

#include "../../fb.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

typedef struct FB_EMUTLS_OBJECT {
	size_t size;
	size_t align;
	void *location;
	const void *initializer;
} FB_EMUTLS_OBJECT;

typedef struct FB_EMUTLS_ENTRY {
	struct FB_EMUTLS_ENTRY *next;
	const FB_EMUTLS_OBJECT *object;
	void *allocation;
	void *value;
} FB_EMUTLS_ENTRY;

static volatile LONG fb_emutls_key_state;
static DWORD fb_emutls_key = TLS_OUT_OF_INDEXES;

static DWORD fb_emutls_get_key( void )
{
	LONG state = InterlockedCompareExchange( &fb_emutls_key_state, 1, 0 );

	if( state == 0 ) {
		DWORD key = TlsAlloc( );
		if( key == TLS_OUT_OF_INDEXES ) {
			InterlockedExchange( &fb_emutls_key_state, -1 );
			abort( );
		}
		fb_emutls_key = key;
		InterlockedExchange( &fb_emutls_key_state, 2 );
	} else {
		while( state == 1 ) {
			Sleep( 0 );
			state = InterlockedCompareExchange( &fb_emutls_key_state, 0, 0 );
		}
		if( state != 2 )
			abort( );
	}

	return fb_emutls_key;
}

void *__emutls_get_address( FB_EMUTLS_OBJECT *object )
{
	FB_EMUTLS_ENTRY *entry;
	DWORD key;
	size_t allocation_size;
	size_t alignment;
	uintptr_t address;
	size_t remainder;

	if( object == NULL )
		return NULL;

	key = fb_emutls_get_key( );
	entry = (FB_EMUTLS_ENTRY *)TlsGetValue( key );
	while( entry != NULL ) {
		if( entry->object == object )
			return entry->value;
		entry = entry->next;
	}

	alignment = object->align;
	if( alignment < sizeof( void * ) )
		alignment = sizeof( void * );
	if( alignment == 0 || (alignment & (alignment - 1)) != 0 )
		abort( );

	if( object->size > SIZE_MAX - (alignment - 1) )
		abort( );
	allocation_size = object->size + alignment - 1;
	if( allocation_size == 0 )
		allocation_size = alignment;

	entry = (FB_EMUTLS_ENTRY *)malloc( sizeof( FB_EMUTLS_ENTRY ) );
	if( entry == NULL )
		abort( );
	entry->allocation = malloc( allocation_size );
	if( entry->allocation == NULL ) {
		free( entry );
		abort( );
	}

	address = (uintptr_t)entry->allocation;
	remainder = (size_t)(address & (uintptr_t)(alignment - 1));
	if( remainder != 0 )
		address += alignment - remainder;
	entry->value = (void *)address;
	entry->object = object;
	if( object->initializer != NULL )
		memcpy( entry->value, object->initializer, object->size );
	else
		memset( entry->value, 0, object->size );

	entry->next = (FB_EMUTLS_ENTRY *)TlsGetValue( key );
	if( !TlsSetValue( key, entry ) ) {
		free( entry->allocation );
		free( entry );
		abort( );
	}

	return entry->value;
}

void fb_hWinCEEmutlsFreeThread( void )
{
	FB_EMUTLS_ENTRY *entry;
	DWORD key;

	if( InterlockedCompareExchange( &fb_emutls_key_state, 0, 0 ) != 2 )
		return;

	key = fb_emutls_key;
	entry = (FB_EMUTLS_ENTRY *)TlsGetValue( key );
	TlsSetValue( key, NULL );
	while( entry != NULL ) {
		FB_EMUTLS_ENTRY *next = entry->next;
		free( entry->allocation );
		free( entry );
		entry = next;
	}
}

/* end of drivers/wince/thread_emutls.c */
