' TEST_MODE : MULTI_MODULE_TEST

#include once "sfx_test_common.bi"

const DUMP_FILE = "sfx/render-tone.tmp"
const MAX_FRAMES = 24000

redim as single samples( 0 to MAX_FRAMES - 1 )

SfxTestUseNullDriver()
SfxTestSetMixerDump( DUMP_FILE, MAX_FRAMES )

sound 440, 9

dim as integer frames = SfxTestLoadDump( DUMP_FILE, samples() )
ASSERT( frames > 20000 )

dim as double hz = SfxTestEstimateZeroCrossHz( samples(), 256, 18000 )
ASSERT( hz > 420.0 and hz < 460.0 )

ASSERT( SfxTestLowEnergyWindows( samples(), 512, 16000, 1024, 0.05 ) = 0 )

'' end of render-tone.bas
