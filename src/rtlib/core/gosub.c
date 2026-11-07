/*
 * Project: FreeBASIC runtime library
 * File: core/gosub.c
 * Purpose: Own the dynamic GOSUB call stack.
 * Responsibilities: Support legacy jump buffers and procedure-local C targets
 *     behind the existing one-pointer compiler context.
 * This file intentionally does NOT contain: ERROR dispatch handlers or expose node layout.
 */
#include "fb.h"
#include "fb_gosub.h"
#include <setjmp.h>

#if defined( _WIN32 ) && defined( __aarch64__ )
void __mingw_longjmp( jmp_buf buf, int value );
#endif

typedef struct gosubnode {
	jmp_buf buf;
	struct gosubnode *next;
	/* Zero selects a legacy jump buffer. C targets are nonzero 32-bit
	 * identifiers, stored in a native word beside the node pointer. */
	uintptr_t target;
} GOSUBNODE;

/* The compiler owns one pointer in automatic storage. Its initializer remains
 * at the original source position; only this module knows the node layout.
 * Contexts are activation-local, so recursion and threads do not share a top. */
FBCALL void *fb_GosubPush( void **context )
{
	GOSUBNODE *node;
	if( context == NULL )
		return NULL;
	node = (GOSUBNODE *)malloc( sizeof( GOSUBNODE ) );
	if( node == NULL )
		return NULL;
	node->next = *context;
	node->target = 0;
	*context = node;
	return &node->buf;
}

FBCALL int fb_GosubPop( void **context )
{
	if( context != NULL && *context != NULL ) {
		GOSUBNODE *node = (GOSUBNODE *)*context;
		*context = node->next;
		free( node );
		return fb_ErrorSetNum( FB_RTERROR_OK );
	}
	return fb_ErrorSetNum( FB_RTERROR_RETURNWITHOUTGOSUB );
}

FBCALL int fb_GosubReturn( void **context )
{
	if( context != NULL && *context != NULL ) {
		GOSUBNODE *node = (GOSUBNODE *)*context;
		jmp_buf buf;
		/* Never interpret a local C target as machine register state. */
		if( node->target != 0 )
			return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
		memset( &buf, 0, sizeof( buf ) );
		memcpy( buf, node->buf, sizeof( buf ) );
		*context = node->next;
		free( node );
#if defined( _WIN32 ) && defined( __aarch64__ )
		__mingw_longjmp( buf, -1 );
#else
		longjmp( buf, -1 );
#endif
	}
	return fb_ErrorSetNum( FB_RTERROR_RETURNWITHOUTGOSUB );
}

FBCALL void fb_GosubExit( void **context )
{
	if( context != NULL ) {
		while( *context != NULL ) {
			GOSUBNODE *node = (GOSUBNODE *)*context;
			*context = node->next;
			free( node );
		}
	}
}

/* Successful push/return preserve ERR, just like the legacy jump-buffer path.
 * RETURN label still uses fb_GosubPop, including its existing ERR reset. */
FBCALL int fb_GosubPushTarget( void **context, unsigned int target )
{
	GOSUBNODE *node;
	if( context == NULL || target == 0 )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	if( fb_GosubPush( context ) == NULL )
		return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
	node = (GOSUBNODE *)*context;
	node->target = target;
	return FB_RTERROR_OK;
}

FBCALL unsigned int fb_GosubReturnTarget( void **context )
{
	GOSUBNODE *node;
	unsigned int target;
	if( context == NULL || *context == NULL ) {
		fb_ErrorSetNum( FB_RTERROR_RETURNWITHOUTGOSUB );
		return 0;
	}
	node = (GOSUBNODE *)*context;
	if( node->target == 0 ) {
		fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
		return 0;
	}
	/* Only PushTarget writes nonzero targets, from this exact 32-bit type. */
	target = (unsigned int)node->target;
	*context = node->next;
	free( node );
	return target;
}

/* end of core/gosub.c */
