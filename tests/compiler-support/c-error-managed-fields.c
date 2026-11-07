/*
 * Project: FreeBASIC managed ERROR ownership qualification
 * File: c-error-managed-fields.c
 * Purpose: Check actual heap retirement through the unchanged allocation ABI.
 * Responsibilities: Track bounded live allocations and close thread-local metadata.
 * This file intentionally does NOT provide cleanup on behalf of BASIC instances.
 */
#include "fb.h"
#include <stdint.h>

void *__real_malloc( size_t bytes );
void *__real_calloc( size_t count, size_t bytes );
void *__real_realloc( void *storage, size_t bytes );
void __real_free( void *storage );
void *__wrap_malloc( size_t bytes );
void *__wrap_calloc( size_t count, size_t bytes );
void *__wrap_realloc( void *storage, size_t bytes );
void __wrap_free( void *storage );
void allocation_begin( void );
int allocation_end( void );
void finish_tls( void );

/* The fixture is single-threaded, including when linked with the MT runtime.
 * The maximum live set is small; a fixed table avoids observer allocations. */
static uintptr_t live[4096];
static size_t allocations, releases, zero_reallocations;
static int active, failed;

static void add( void *storage )
{
    size_t index;
    if( !active || storage == NULL ) return;
    for( index = 0; index < sizeof( live ) / sizeof( live[0] ); ++index ) {
        if( live[index] == 0 ) {
            live[index] = (uintptr_t)storage;
            ++allocations;
            return;
        }
    }
    failed = 1;
}

static int remove_address( uintptr_t address )
{
    size_t index;
    if( address == 0 ) return 0;
    for( index = 0; index < sizeof( live ) / sizeof( live[0] ); ++index ) {
        if( live[index] == address ) {
            live[index] = 0;
            ++releases;
            return 1;
        }
    }
    return 0;
}

void *__wrap_malloc( size_t bytes )
{
    void *storage = __real_malloc( bytes );
    add( storage );
    return storage;
}

void *__wrap_calloc( size_t count, size_t bytes )
{
    void *storage = __real_calloc( count, bytes );
    add( storage );
    return storage;
}

void *__wrap_realloc( void *storage, size_t bytes )
{
    /* Retire the old numeric identity before realloc may invalidate it.
     * On failure with a nonzero size, restore its still-live identity. */
    uintptr_t previous = (uintptr_t)storage;
    int tracked = active ? remove_address( previous ) : 0;
    void *result = __real_realloc( storage, bytes );
    if( active && bytes == 0 ) ++zero_reallocations;
    if( result != NULL )
        add( result );
    else if( tracked && bytes != 0 )
        add( (void *)previous );
    return result;
}

void __wrap_free( void *storage )
{
    if( active ) remove_address( (uintptr_t)storage );
    __real_free( storage );
}

void allocation_begin( void )
{
    memset( live, 0, sizeof( live ) );
    allocations = releases = zero_reallocations = 0;
    failed = 0;
    active = 1;
}

int allocation_end( void )
{
    size_t index, outstanding = 0;
    active = 0;
    for( index = 0; index < sizeof( live ) / sizeof( live[0] ); ++index )
        outstanding += live[index] != 0;
    printf( "MANAGED_ALLOCATION_RESULT %zu %zu %zu %zu\n",
            allocations, releases, outstanding, zero_reallocations );
    return failed || outstanding != 0 || allocations == 0 ||
           allocations != releases || zero_reallocations != 0;
}

void finish_tls( void )
{
    fb_TlsFreeCtxTb();
}

/* end of c-error-managed-fields.c */
