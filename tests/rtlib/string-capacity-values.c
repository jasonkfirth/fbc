/*
 * Project: FreeBASIC runtime library tests
 * File: string-capacity-values.c
 * Purpose: Check ordinary string growth and deterministic allocation failures.
 * Responsibilities: Verify capacity math, preserved contents and temporary cleanup.
 * This file intentionally does NOT allocate large buffers or replace string logic.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fb.h"
#include "strings/str_size.h"

FB_RTLIB_CTX fb_ctx;
static int failures_left, lock_depth, lock_calls;
static unsigned int checks, allocations;
static size_t requested;
static uint64_t normal_hash = UINT64_C(14695981039346656037);

void *capacity_malloc( size_t bytes );
void *capacity_realloc( void *pointer, size_t bytes );

static void check( int condition, const char *name )
{
    if( !condition ) {
        fprintf( stderr, "string capacity failure: %s\n", name );
        exit( 1 );
    }
    checks += 1;
}

void *capacity_malloc( size_t bytes )
{
    allocations += 1;
    requested = bytes;
    if( failures_left > 0 ) {
        failures_left -= 1;
        return NULL;
    }
    /* A count-boundary test must never become a large real allocation. */
    check( bytes <= 4096, "fixture allocation bound" );
    return malloc( bytes );
}

void *capacity_realloc( void *pointer, size_t bytes )
{
    allocations += 1;
    requested = bytes;
    if( failures_left > 0 ) {
        failures_left -= 1;
        return NULL;
    }
    check( bytes <= 4096, "fixture reallocation bound" );
    return realloc( pointer, bytes );
}

#ifdef ENABLE_MT
/* These hooks check entry/exit balance in the single-threaded failure fixture.
   Linked normal-library checks separately exercise the real MT runtime. */
void FBCALL fb_StrLock( void )
{
    lock_depth += 1;
    lock_calls += 1;
}

void FBCALL fb_StrUnlock( void )
{
    check( lock_depth > 0, "matching string unlock" );
    lock_depth -= 1;
}
#endif

static void normal_values( void )
{
    ssize_t length;
    check( fb_StrConcat( NULL, NULL, 0, NULL, 0 ) == NULL, "existing empty concatenation result" );
    for( length = 0; length <= 130; ++length ) {
        FBSTRING value = { NULL, 0, 0 };
        ssize_t expected = (length + 31) & ~(ssize_t)31;
        ssize_t expanded = length + 65;
        ssize_t grown = (expanded + 31) & ~(ssize_t)31;
        size_t index;
        grown += grown >> 3;
        check( fb_hStrAlloc( &value, length ) == &value, "ordinary allocation" );
        check( value.len == length && value.size == expected && requested == (size_t)expected + 1,
            "ordinary rounding policy" );
        memset( value.data, 'A', (size_t)length );
        value.data[length] = 0;
        check( fb_hStrRealloc( &value, expanded, TRUE ) == &value, "ordinary preserving growth" );
        check( value.len == expanded && value.size == grown, "ordinary growth policy" );
        for( index = 0; index < (size_t)length; ++index )
            check( value.data[index] == 'A', "preserved prefix" );
        normal_hash = (normal_hash ^ (uint64_t)value.size) * UINT64_C(1099511628211);
        normal_hash = (normal_hash ^ (uint64_t)value.len) * UINT64_C(1099511628211);
        fb_StrDelete( &value );
        check( value.data == NULL && value.len == 0 && value.size == 0, "ordinary delete" );
    }
    {
        FBSTRING value = { NULL, 0, 0 };
        FBSTRING *joined;
        const char source[] = "abc";
        check( fb_StrInit( &value, -1, (void *)source, 4, FALSE ) == &value && strcmp( value.data, "abc" ) == 0,
            "ordinary initialization" );
        check( fb_StrConcatAssign( &value, -1, (void *)"DEF", 4, FALSE ) == &value && strcmp( value.data, "abcDEF" ) == 0,
            "ordinary append" );
        check( fb_StrConcatByref( &value, -1, &value, -1, FALSE ) == &value && strcmp( value.data, "abcDEFabcDEF" ) == 0,
            "ordinary self append" );
        joined = fb_StrConcat( NULL, &value, -1, (void *)"!", 2 );
        check( joined != NULL && strcmp( joined->data, "abcDEFabcDEF!" ) == 0 && FB_ISTEMP( joined ),
            "ordinary concatenation temporary" );
        fb_hStrDelTemp( joined );
        fb_StrDelete( &value );
    }
}

static void fallback_values( void )
{
    FBSTRING value = { NULL, 0, 0 };
    failures_left = 1;
    check( fb_hStrAlloc( &value, 17 ) == &value && value.len == 17 && value.size == 17 && requested == 18,
        "exact-size allocation fallback" );
    memcpy( value.data, "abcdefghijklmnopq", 18 );
    failures_left = 1;
    check( fb_hStrRealloc( &value, 80, TRUE ) == &value && value.len == 80 && value.size == 80 && requested == 81,
        "exact-size reallocation fallback" );
    check( memcmp( value.data, "abcdefghijklmnopq", 17 ) == 0, "fallback preserves prefix" );
    fb_StrDelete( &value );
    failures_left = 0;
}

static void pool_values( void )
{
    FBSTRING_ACCESS *items[FB_STR_TMPDESCRIPTORS];
    unsigned int index;
    for( index = 0; index < FB_STR_TMPDESCRIPTORS; ++index ) {
        items[index] = fb_hStrAllocTempDesc();
        check( items[index] != NULL, "temporary pool has no leaked descriptor" );
    }
    check( fb_hStrAllocTempDesc() == NULL, "temporary pool bound" );
    for( index = 0; index < FB_STR_TMPDESCRIPTORS; ++index )
        check( fb_hStrDelTempDesc( items[index] ) == 0, "temporary pool release" );
}

static void failure_values( void )
{
    FBSTRING value = { NULL, 0, 0 }, saved;
    FBSTRING_ACCESS *temporary, *joined;
    ssize_t capacity = 99, total = 99;
    unsigned int before;
    char source[65];
    memset( source, 'Z', 64 );source[64] = 0;

    check( !fb_hStrCapacity( -1, TRUE, &capacity ) && capacity == 99, "negative size rejected" );
    check( !fb_hStrCapacity( FB_STRSIZEMSK, TRUE, &capacity ), "terminator remains representable" );
    check( fb_hStrCapacity( FB_STRSIZEMSK - 1, TRUE, &capacity ) && capacity == FB_STRSIZEMSK - 1,
        "capacity limit avoids optional rounding and growth" );
    check( !fb_hStrAddSize( FB_STRSIZEMSK - 1, 1, &total ) && total == 99, "sum checked before addition" );
    check( fb_hStrAddSize( FB_STRSIZEMSK - 2, 1, &total ) && total == FB_STRSIZEMSK - 1,
        "largest representable concatenation" );
    before = allocations;
    check( fb_hStrAlloc( NULL, 1 ) == NULL && fb_hStrAlloc( &value, -1 ) == NULL &&
        fb_hStrRealloc( &value, FB_STRSIZEMSK, TRUE ) == NULL && allocations == before,
        "invalid sizes do not reach allocator" );

    check( fb_hStrAlloc( &value, 3 ) == &value, "failure fixture setup" );
    memcpy( value.data, "abc", 4 );saved = value;
    failures_left = 2;
    check( fb_hStrRealloc( &value, 80, TRUE ) == NULL, "preserving failure returned" );
    check( value.data == saved.data && value.len == saved.len && value.size == saved.size && strcmp( value.data, "abc" ) == 0,
        "preserving failure retains descriptor and contents" );
    failures_left = 2;
    check( fb_StrConcatAssign( &value, -1, source, 65, FALSE ) == &value &&
        value.data == saved.data && value.len == 3 && strcmp( value.data, "abc" ) == 0,
        "append failure does not copy" );
    failures_left = 2;
    check( fb_StrAssign( &value, -1, source, 65, FALSE ) == &value &&
        value.data == NULL && value.len == 0 && value.size == 0, "assignment failure does not copy" );

    failures_left = 2;
    check( fb_StrInit( &value, -1, source, 65, FALSE ) == &value &&
        value.data == NULL && value.len == 0 && value.size == 0, "initialization failure does not copy" );
    failures_left = 0;
    check( fb_hStrAlloc( &value, 40 ) == &value, "self append setup" );
    memset( value.data, 'B', 40 );value.data[40] = 0;saved = value;
    failures_left = 2;
    check( fb_StrConcatByref( &value, -1, &value, -1, FALSE ) == &value &&
        value.data == saved.data && value.len == 40 && value.data[39] == 'B' && value.data[40] == 0,
        "self append failure preserves contents" );
    fb_StrDelete( &value );
    failures_left = 0;
    temporary = fb_hStrAllocTemp( NULL, 64 );
    check( temporary != NULL && FB_ISTEMP( temporary ), "source temporary setup" );
    memcpy( temporary->data, source, 65 );
    failures_left = 2;
    joined = fb_StrConcat( NULL, temporary, -1, (void *)"!", 2 );
    check( joined == NULL && temporary->data == NULL && !FB_ISTEMP( temporary ),
        "concatenation failure retires source temporary" );
    failures_left = 0;
    temporary = fb_hStrAllocTemp( NULL, 4 );
    check( temporary != NULL, "supplied output temporary setup" );
    memcpy( temporary->data, "old!", 5 );
    failures_left = 2;
    joined = fb_StrConcat( temporary, source, 65, (void *)"!", 2 );
    check( joined == temporary && joined->data == NULL && joined->len == 0,
        "failed supplied output retains descriptor ownership" );
    fb_hStrDelTemp( joined );
    failures_left = 0;
    pool_values();
    check( lock_depth == 0, "failure paths balance string locks" );
}

int main( int argc, char **argv )
{
    int after;
    check( argc == 2, "phase argument" );
    after = strcmp( argv[1], "after" ) == 0;
    check( after || strcmp( argv[1], "before" ) == 0, "recognized phase" );
    normal_values();
    fallback_values();
    pool_values();
    if( after )
        failure_values();
    check( lock_depth == 0, "all locks released" );
#ifdef ENABLE_MT
    check( lock_calls > 0, "MT path exercised lock hooks" );
#else
    check( lock_calls == 0, "ST path has no lock hooks" );
#endif
    printf( "STRING_CAPACITY_VALUES_PASS phase=%s checks=%u normal=%016llx\n",
        argv[1], checks, (unsigned long long)normal_hash );
    return 0;
}

/* end of string-capacity-values.c */
