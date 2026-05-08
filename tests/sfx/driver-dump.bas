' TEST_MODE : MULTI_MODULE_TEST

#include once "sfx_test_common.bi"

const DUMP_FILE = "sfx/driver-dump.tmp"
const MAX_FRAMES = 6000

redim as single samples( 0 to MAX_FRAMES - 1 )

SfxTestUseNullDriver()
SfxTestSetDriverDump( DUMP_FILE, MAX_FRAMES )

sound 0, 440, 0.10, 0.60
fb_sfxUpdate( 5000 )

dim as integer frames = SfxTestLoadDump( DUMP_FILE, samples() )
ASSERT( frames >= 5000 )
ASSERT( SfxTestRms( samples(), 256, 2048 ) > 0.05 )

'' end of driver-dump.bas
