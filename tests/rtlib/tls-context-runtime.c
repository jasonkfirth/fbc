/*
 * Project: FreeBASIC runtime tests
 * File: tls-context-runtime.c
 * Purpose: Check explicit TLS context destruction through the runtime archive.
 * Responsibilities: Check zero initialization, reuse and one destructor call.
 * Scope: The calling thread's graphics TLS slot in a headless test process.
 */
#include "fb.h"

static int checks, destroyed;

static void check( int condition )
{
	++checks;
	if( !condition ) {
		fprintf( stderr, "TLS_CONTEXT_RUNTIME_FAIL check=%d\n", checks );
		exit( 1 );
	}
}

static void destroy( void *context )
{
	int *value = context;
	check( *value == 123 );
	++destroyed;
}

int main( int argc, char **argv )
{
	int *value;
	fb_Init( argc, argv, 0 );
	check( fb_TlsGetCtxExisting( FB_TLSKEY_GFX ) == NULL );
	value = fb_TlsGetCtx( FB_TLSKEY_GFX, sizeof( *value ), destroy );
	check( value != NULL );
	check( *value == 0 );
	*value = 123;
	check( fb_TlsGetCtx( FB_TLSKEY_GFX, sizeof( *value ), destroy ) == value );
	fb_TlsDelCtx( FB_TLSKEY_GFX );
	check( destroyed == 1 );
	check( fb_TlsGetCtxExisting( FB_TLSKEY_GFX ) == NULL );
	fb_TlsDelCtx( FB_TLSKEY_GFX );
	check( destroyed == 1 );
	value = fb_TlsGetCtx( FB_TLSKEY_GFX, sizeof( *value ), NULL );
	check( value != NULL );
	check( *value == 0 );
	fb_TlsDelCtx( FB_TLSKEY_GFX );
	check( destroyed == 1 );
	check( fb_TlsGetCtxExisting( FB_TLSKEY_GFX ) == NULL );
	printf( "TLS_CONTEXT_RUNTIME_PASS checks=%d\n", checks );
	return 0;
}

/* end of tls-context-runtime.c */
