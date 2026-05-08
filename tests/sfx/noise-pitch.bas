' TEST_MODE : MULTI_MODULE_TEST

#include once "sfx_test_common.bi"

const DUMP_FILE = "sfx/noise-pitch.tmp"
const MAX_FRAMES = 12000

redim as single samples( 0 to MAX_FRAMES - 1 )

SfxTestUseNullDriver()
SfxTestSetMixerDump( DUMP_FILE, MAX_FRAMES )

noise 0, 100, 0.20, 0.80
fb_sfxUpdate( 9000 )

dim as integer frames = SfxTestLoadDump( DUMP_FILE, samples() )
ASSERT( frames >= 9000 )

dim as integer changes = SfxTestCountChanges( samples(), 512, 8192, 0.001 )

ASSERT( changes >= 10 )
ASSERT( changes <= 30 )

'' end of noise-pitch.bas
