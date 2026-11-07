/*
 * Project: FreeBASIC runtime tests
 * File: rnd-state-values.c
 * Purpose: Check seed-vector recurrence, output bounds and empty fills.
 * Responsibilities: Compare independent modular arithmetic and preserve guards.
 * This file intentionally does NOT test unrelated RND providers or platform APIs.
 */
#ifdef __cplusplus
extern "C" {
#endif
#include "fb.h"
#ifdef __cplusplus
}
#endif

#ifdef PRIVATE_RND_STATE
#include "math/rnd_state.h"
#endif

#define CHECK(condition) do { if( !(condition) ) { \
    printf( "RND state failure at line %d\n", __LINE__ ); return 1; \
} } while( 0 )

int main( void )
{
    static const uint32_t seeds[] = {0, 1, UINT32_MAX, UINT32_C(0x80000000), 327680, 19650218};
    static const uint32_t lengths[] = {1, 2, 623, 624, 625, 1000};
    uint32_t words[1002];
    size_t seed, length, i;

    for( seed = 0; seed < sizeof( seeds ) / sizeof( seeds[0] ); seed++ ) {
        for( length = 0; length < sizeof( lengths ) / sizeof( lengths[0] ); length++ ) {
            uint64_t reference = seeds[seed];
            for( i = 0; i < sizeof( words ) / sizeof( words[0] ); i++ )
                words[i] = UINT32_C(0xa5a55a5a);
            hRnd_FillFAST32( words + 1, lengths[length], seeds[seed] );
            CHECK( words[0] == UINT32_C(0xa5a55a5a) );
            for( i = 0; i < lengths[length]; i++ ) {
                CHECK( words[i + 1] == (uint32_t)reference );
                /* Use wider arithmetic and an explicit modulus as the oracle. */
                reference = (reference * UINT64_C(1664525) + UINT64_C(1013904223)) % UINT64_C(4294967296);
            }
            for( i = (size_t)lengths[length] + 1; i < sizeof( words ) / sizeof( words[0] ); i++ )
                CHECK( words[i] == UINT32_C(0xa5a55a5a) );
        }
    }

    words[0] = 11;
    words[1] = 22;
    hRnd_FillFAST32( words, 0, 123 );
#ifdef PRIVATE_RND_STATE
    CHECK( words[0] == 11 && words[1] == 22 );
    hRnd_FillFAST32( NULL, 0, 123 );
#else
    /* Record the old empty-fill write without probing an invalid pointer. */
    CHECK( words[0] == 123 && words[1] == 22 );
#endif
    puts( "RND_STATE_VALUES_PASS cases=36" );
    return 0;
}

/* end of rnd-state-values.c */
