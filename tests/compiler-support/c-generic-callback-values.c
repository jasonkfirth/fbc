/*
 * Project: FreeBASIC generic callback representation qualification
 * File: c-generic-callback-values.c
 * Purpose: Model a foreign API that stores a generic callback and restores its type.
 * Responsibilities: Invoke only exact int(int,void*) callbacks and preserve code addresses.
 * This file intentionally does NOT call through the erased signature.
 */
#include <stddef.h>
#include <string.h>

typedef void (*erased_callback)( void );
typedef int (*typed_callback)( int, void * );
typedef long long (*wide_callback)( int, void * );

int callback_probe( erased_callback callback, int value, void *userdata );
erased_callback callback_echo( erased_callback callback );
erased_callback callback_get( void );
typed_callback callback_null( void );
int callback_is_null( erased_callback callback );
int callback_gate( void );
int callback_typed_sink( wide_callback callback );

/* The supported BASIC ABI represents each procedure pointer in one code-address
 * slot. The foreign API owns the signature knowledge; erasure never invokes it. */
typedef char callback_size_check[sizeof(erased_callback) == sizeof(typed_callback) ? 1 : -1];

int callback_probe( erased_callback callback, int value, void *userdata )
{
	typed_callback restored;
	if( callback == NULL )
		return -1;
	memcpy( &restored, &callback, sizeof( restored ) );
	return restored( value, userdata );
}

erased_callback callback_echo( erased_callback callback )
{
	return callback;
}

static int foreign_callback( int value, void *userdata )
{
	return value + *(int *)userdata + 10;
}

erased_callback callback_get( void )
{
	typed_callback callback = foreign_callback;
	erased_callback erased;
	memcpy( &erased, &callback, sizeof( erased ) );
	return erased;
}

typed_callback callback_null( void )
{
	return NULL;
}

int callback_is_null( erased_callback callback )
{
	return callback == NULL;
}

int callback_gate( void )
{
	return 0;
}

int callback_typed_sink( wide_callback callback )
{
	return callback != NULL;
}

/* end of c-generic-callback-values.c */
