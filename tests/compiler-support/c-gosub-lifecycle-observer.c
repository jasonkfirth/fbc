/*
 * Project: FreeBASIC local C GOSUB qualification
 * File: c-gosub-lifecycle-observer.c
 * Purpose: Observe only GOSUB allocations in the single-threaded BASIC fixture.
 * Responsibilities: Inject one refusal and prove owner-exit node balance.
 * This file intentionally does NOT replace the process allocator or thread API.
 */
#include "fb.h"

void *gosub_test_malloc( size_t bytes );
void gosub_test_free( void *pointer );
void gosub_test_arm( int fail_next );
void gosub_test_balanced( void );

static unsigned int allocations;
static unsigned int releases;
static int refuse_next;

void *gosub_test_malloc( size_t bytes )
{
    void *pointer;
    if( refuse_next ) {
        refuse_next = 0;
        return NULL;
    }
    pointer = malloc( bytes );
    if( pointer != NULL )
        allocations++;
    return pointer;
}

void gosub_test_free( void *pointer )
{
    if( pointer != NULL )
        releases++;
    free( pointer );
}

void gosub_test_arm( int fail_next )
{
    refuse_next = fail_next;
}

void gosub_test_balanced( void )
{
    if( allocations != 1000 || releases != allocations )
        exit( 87 );
}

/* end of c-gosub-lifecycle-observer.c */
