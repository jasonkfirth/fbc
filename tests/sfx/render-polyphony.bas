' TEST_MODE : MULTI_MODULE_TEST

#include once "sfx_test_common.bi"

const DUMP_FILE = "sfx/render-polyphony.tmp"
const MAX_FRAMES = 14000

redim as single samples( 0 to MAX_FRAMES - 1 )

SfxTestUseNullDriver()
SfxTestSetMixerDump( DUMP_FILE, MAX_FRAMES )

sound 0, 440, 0.25, 0.45
sound 1, 660, 0.25, 0.45
fb_sfxUpdate( 12000 )

dim as integer frames = SfxTestLoadDump( DUMP_FILE, samples() )
ASSERT( frames >= 12000 )

dim as double p440 = SfxTestBandPower( samples(), 512, 8192, 440.0 )
dim as double p660 = SfxTestBandPower( samples(), 512, 8192, 660.0 )
dim as double p550 = SfxTestBandPower( samples(), 512, 8192, 550.0 )

ASSERT( p440 > 0.02 )
ASSERT( p660 > 0.02 )
ASSERT( p440 > p550 * 1.5 )
ASSERT( p660 > p550 * 1.5 )

'' end of render-polyphony.bas
