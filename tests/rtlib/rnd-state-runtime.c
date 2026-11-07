/*
 * Project: FreeBASIC runtime tests
 * File: rnd-state-runtime.c
 * Purpose: Check archive-backed RND state and reproducible output sequences.
 * Responsibilities: Verify the seed vector, FAST32 arithmetic and repeatability.
 * This file intentionally does NOT test unrelated providers or concurrent callers.
 */
#ifdef __cplusplus
extern "C" {
#endif
#include "fb.h"
#ifdef __cplusplus
}
#endif
#include <inttypes.h>

#define CHECK(condition) do { if( !(condition) ) { \
    printf( "RND runtime failure at line %d\n", __LINE__ ); return 1; \
} } while( 0 )

int main( int argc, char **argv )
{
    static const uint32_t seeds[] = {0, 1, UINT32_MAX, UINT32_C(0x80000000), 327680, 19650218};
    static const int algorithms[] = {FB_RND_FAST, FB_RND_MTWIST};
    uint32_t output[4096];
    size_t algorithm, seed, i;

    fb_Init( argc, argv, 0 );
    for( algorithm = 0; algorithm < sizeof( algorithms ) / sizeof( algorithms[0] ); algorithm++ ) {
        for( seed = 0; seed < sizeof( seeds ) / sizeof( seeds[0] ); seed++ ) {
            uint64_t reference = seeds[seed];
            uint64_t checksum = UINT64_C(14695981039346656037);
            FB_RNDSTATE *state;
            fb_Randomize( (double)seeds[seed], algorithms[algorithm] );
            state = fb_RndGetState();
            CHECK( state != NULL && state->algorithm == (uint32_t)algorithms[algorithm] );
            CHECK( state->rndproc != NULL && state->rndproc32 != NULL );
            if( algorithms[algorithm] == FB_RND_MTWIST ) {
                CHECK( state->length == FB_RND_MAX_STATE );
                for( i = 0; i < FB_RND_MAX_STATE; i++ ) {
                    CHECK( state->state32[i] == (uint32_t)reference );
                    reference = (reference * UINT64_C(1664525) + UINT64_C(1013904223)) % UINT64_C(4294967296);
                }
            }
            for( i = 0; i < sizeof( output ) / sizeof( output[0] ); i++ ) {
                output[i] = fb_Rnd32();
                if( algorithms[algorithm] == FB_RND_FAST ) {
                    reference = (reference * UINT64_C(1664525) + UINT64_C(1013904223)) % UINT64_C(4294967296);
                    CHECK( output[i] == (uint32_t)reference );
                }
                checksum = (checksum ^ output[i]) * UINT64_C(1099511628211);
            }
            fb_Randomize( (double)seeds[seed], algorithms[algorithm] );
            for( i = 0; i < sizeof( output ) / sizeof( output[0] ); i++ )
                CHECK( fb_Rnd32() == output[i] );
            printf( "%d %" PRIu32 " %" PRIu64 "\n", algorithms[algorithm], seeds[seed], checksum );
        }
    }
    puts( "RND_STATE_RUNTIME_PASS cases=12" );
    return 0;
}

/* end of rnd-state-runtime.c */
