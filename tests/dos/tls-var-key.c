/*
 * Project: FreeBASIC DOS qualification
 * File: tls-var-key.c
 * Purpose: Verify named and private runtime TLS identities.
 * Responsibilities: Check initialization, reuse, alignment and context cleanup.
 * This file does not create threads or depend on compiler-generated TLS code.
 */
#include "fb.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned char first_key, second_key;
static uint32_t first_initial = 17, second_initial = 41;
static int checks;

static void check( int condition )
{
	++checks;
	if( !condition ) {
		fprintf( stderr, "DOS_TLS_KEY_FAIL check=%d\n", checks );
		exit( 1 );
	}
}

int main( int argc, char **argv )
{
	uint32_t *first, *second, *named;
	char same_name[] = "named";
	fb_Init( argc, argv, 0 );
	first = fb_TlsGetVarKey( &first_key, sizeof(*first), 32, (uintptr_t)&first_initial );
	second = fb_TlsGetVarKey( &second_key, sizeof(*second), 32, (uintptr_t)&second_initial );
	check( first != second );
	check( *first == 17 && *second == 41 );
	/* This deliberately exceeds uint32_t's natural alignment. */
	check( (uintptr_t)first % 32 == 0 && (uintptr_t)second % 32 == 0 );
	*first = 99;
	check( fb_TlsGetVarKey( &first_key, sizeof(*first), 32, 0 ) == first );
	check( *first == 99 && first_initial == 17 );
	named = fb_TlsGetVar( "named", sizeof(*named), 4, (uintptr_t)&first_initial );
	check( named != first && named != second && *named == 17 );
	check( fb_TlsGetVar( same_name, sizeof(*named), 4, 0 ) == named );
	fb_TlsDelCtx( FB_TLSKEY_USERVAR );
	first = fb_TlsGetVarKey( &first_key, sizeof(*first), 32, (uintptr_t)&first_initial );
	check( *first == 17 );
	fb_TlsDelCtx( FB_TLSKEY_USERVAR );
	printf( "DOS_TLS_KEY_PASS checks=%d\n", checks );
	return 0;
}

/* end of tls-var-key.c */
