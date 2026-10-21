/*
 * Project: FreeBASIC runtime library
 * File: threads/thread_var.c
 * Purpose: Provide runtime storage for compiler-emulated thread-local variables.
 * Responsibilities: Allocate, initialize, and release per-thread variable slots.
 * This file intentionally does NOT contain: Compiler AST lowering or thread creation.
 */
#include "../fb.h"
#include "../fb_private_thread.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct FB_TLS_USER_VAR {
	struct FB_TLS_USER_VAR *next;
	char *name;
	void *allocation;
	void *value;
	size_t size;
	size_t align;
} FB_TLS_USER_VAR;

typedef struct FB_TLS_USER_VAR_CTX {
	FB_TLS_USER_VAR *vars;
} FB_TLS_USER_VAR_CTX;

/*
    Generated variable accesses use the returned address directly. If a slot
    cannot be prepared, abort instead of exposing a null or shared fallback.
*/
static void *hTlsUserVarAllocationFailure( void )
{
	abort( );
	return NULL;
}

static void hFreeTlsUserVars( void *data )
{
	FB_TLS_USER_VAR_CTX *ctx = (FB_TLS_USER_VAR_CTX *)data;
	FB_TLS_USER_VAR *var = ctx->vars;

	while( var != NULL ) {
		FB_TLS_USER_VAR *next = var->next;
		free( var->allocation );
		free( var->name );
		free( var );
		var = next;
	}
}

FBCALL void *fb_TlsGetVar( const char *name, unsigned int len,
	unsigned int align, uintptr_t initial_address )
{
	FB_TLS_USER_VAR_CTX *ctx;
	FB_TLS_USER_VAR *var;
	size_t name_len;
	size_t allocation_len;
	void *allocation;
	uintptr_t aligned_address;
	const void *initial = (const void *)initial_address;

	if( (name == NULL) || (len == 0) || (align == 0) ||
	    (((uintptr_t)align & ((uintptr_t)align - 1)) != 0) ) {
		return hTlsUserVarAllocationFailure( );
	}

	name_len = strlen( name );
	if( (name_len == 0) || (name_len == (size_t)-1) ) {
		return hTlsUserVarAllocationFailure( );
	}

	ctx = (FB_TLS_USER_VAR_CTX *)fb_TlsGetCtx(
		FB_TLSKEY_USERVAR, sizeof( FB_TLS_USER_VAR_CTX ), hFreeTlsUserVars );
	if( ctx == NULL ) {
		return hTlsUserVarAllocationFailure( );
	}

	for( var = ctx->vars; var != NULL; var = var->next ) {
		if( strcmp( var->name, name ) == 0 ) {
			if( (var->size != (size_t)len) || (var->align != (size_t)align) ) {
				return hTlsUserVarAllocationFailure( );
			}
			return var->value;
		}
	}

	if( (size_t)len > (size_t)-1 - ((size_t)align - 1) ) {
		return hTlsUserVarAllocationFailure( );
	}
	allocation_len = (size_t)len + (size_t)align - 1;
	allocation = malloc( allocation_len );
	if( allocation == NULL ) {
		return hTlsUserVarAllocationFailure( );
	}

	aligned_address = ((uintptr_t)allocation + (uintptr_t)align - 1) &
		~((uintptr_t)align - 1);
	var = (FB_TLS_USER_VAR *)calloc( 1, sizeof( FB_TLS_USER_VAR ) );
	if( var == NULL ) {
		free( allocation );
		return hTlsUserVarAllocationFailure( );
	}
	var->name = (char *)malloc( name_len + 1 );
	if( var->name == NULL ) {
		free( var );
		free( allocation );
		return hTlsUserVarAllocationFailure( );
	}
	memcpy( var->name, name, name_len + 1 );
	var->allocation = allocation;
	var->value = (void *)aligned_address;
	var->size = len;
	var->align = align;
	if( initial != NULL ) {
		memcpy( var->value, initial, len );
	} else {
		memset( var->value, 0, len );
	}
	var->next = ctx->vars;
	ctx->vars = var;

	return var->value;
}

/* end of threads/thread_var.c */
