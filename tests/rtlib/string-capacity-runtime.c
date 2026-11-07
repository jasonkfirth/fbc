/*
 * Project: FreeBASIC runtime library tests
 * File: string-capacity-runtime.c
 * Purpose: Check string storage through normal ST/MT runtime archives.
 * Responsibilities: Exercise assignment, concatenation, limits and joined workers.
 * This file intentionally does NOT replace allocators or use large allocations.
 */
#include <stdio.h>
#include <string.h>
#include "fb.h"
#include "strings/str_size.h"

static int string_cycle( void )
{
    FBSTRING left = { NULL, 0, 0 }, right = { NULL, 0, 0 }, result = { NULL, 0, 0 };
    FBSTRING_ACCESS *temporary = NULL;
    int success = FALSE;
    fb_StrInit( &left, -1, (void *)"ab", 3, FALSE );
    fb_StrInit( &right, -1, (void *)"CD", 3, FALSE );
    if( left.data == NULL || right.data == NULL )
        goto cleanup;
    temporary = fb_StrConcat( NULL, &left, -1, &right, -1 );
    if( temporary == NULL || temporary->data == NULL )
        goto cleanup;
    fb_StrAssign( &result, -1, temporary, -1, FALSE );
    temporary = NULL;
    if( result.data == NULL )
        goto cleanup;
    fb_StrConcatAssign( &result, -1, (void *)"!", 2, FALSE );
    fb_StrConcatByref( &result, -1, &result, -1, FALSE );
    success = FB_STRSIZE( &left ) == 2 && FB_STRSIZE( &right ) == 2 &&
        FB_STRSIZE( &result ) == 10 && strcmp( result.data, "abCD!abCD!" ) == 0;
cleanup:
    if( temporary != NULL )
        fb_hStrDelTemp( temporary );
    fb_StrDelete( &left );
    fb_StrDelete( &right );
    fb_StrDelete( &result );
    return success;
}

#ifdef ENABLE_MT
typedef struct STRING_WORKER {
    int success;
} STRING_WORKER;

/* Each worker owns its strings and result slot. The runtime string lock
   protects the shared temporary-descriptor pool; the parent reads each result
   only after fb_ThreadWait has joined that worker. */
static void FBCALL string_worker( void *argument )
{
    STRING_WORKER *worker = (STRING_WORKER *)argument;
    int index;
    worker->success = TRUE;
    for( index = 0; index < 64; ++index ) {
        if( !string_cycle() ) {
            worker->success = FALSE;
            break;
        }
    }
}
#endif

int main( void )
{
    FBSTRING value = { NULL, 0, 0 }, saved;
    FBSTRING_ACCESS *items[FB_STR_TMPDESCRIPTORS];
    ssize_t capacity;
    int index, available = 0, pool_ok = TRUE;
    if( !fb_hStrCapacity( FB_STRSIZEMSK - 1, TRUE, &capacity ) || capacity != FB_STRSIZEMSK - 1 )
        return 1;
    if( fb_hStrAlloc( &value, 3 ) != &value )
        return 2;
    memcpy( value.data, "abc", 4 );saved = value;
    if( fb_hStrRealloc( &value, -1, TRUE ) != NULL || fb_hStrRealloc( &value, FB_STRSIZEMSK, TRUE ) != NULL ||
        value.data != saved.data || value.len != saved.len || value.size != saved.size || strcmp( value.data, "abc" ) != 0 )
        return 3;
    fb_StrDelete( &value );
    for( index = 0; index < 64; ++index ) {
        if( !string_cycle() )
            return 4;
    }
#ifdef ENABLE_MT
    {
        STRING_WORKER workers[2] = { { FALSE }, { FALSE } };
        FBTHREAD *threads[2];
        threads[0] = fb_ThreadCreate( string_worker, &workers[0], 0 );
        threads[1] = fb_ThreadCreate( string_worker, &workers[1], 0 );
        if( threads[0] != NULL ) fb_ThreadWait( threads[0] );
        if( threads[1] != NULL ) fb_ThreadWait( threads[1] );
        if( threads[0] == NULL || threads[1] == NULL || !workers[0].success || !workers[1].success )
            return 5;
    }
#endif
    FB_STRLOCK();
    for( index = 0; index < FB_STR_TMPDESCRIPTORS; ++index ) {
        items[index] = fb_hStrAllocTempDesc();
        if( items[index] == NULL )
            break;
        available += 1;
    }
    if( available == FB_STR_TMPDESCRIPTORS ) {
        FBSTRING_ACCESS *extra = fb_hStrAllocTempDesc();
        if( extra != NULL ) {
            fb_hStrDelTempDesc( extra );
            pool_ok = FALSE;
        }
    }
    for( index = 0; index < available; ++index ) {
        if( fb_hStrDelTempDesc( items[index] ) != 0 )
            pool_ok = FALSE;
    }
    FB_STRUNLOCK();
    if( available != FB_STR_TMPDESCRIPTORS || !pool_ok )
        return 6;
    puts( "STRING_CAPACITY_RUNTIME_PASS" );
    return 0;
}

/* end of string-capacity-runtime.c */
