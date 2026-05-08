' TEST_MODE : MULTI_MODULE_TEST

#include once "sfx_test_common.bi"

const WAV_FILE = "sfx/sample-pitch.wav"
const DUMP_FILE = "sfx/sample-pitch.tmp"
const MAX_FRAMES = 10000

redim as single samples( 0 to MAX_FRAMES - 1 )

SfxTestUseNullDriver()
SfxTestSetMixerDump( DUMP_FILE, MAX_FRAMES )
SfxTestWriteSineWav( WAV_FILE, 22050, 440.0, 4410 )

sfx load 1, WAV_FILE
sfx play 0, 1, 2.0
fb_sfxUpdate( 8000 )

dim as integer frames = SfxTestLoadDump( DUMP_FILE, samples() )
ASSERT( frames >= 8000 )

dim as double hz = SfxTestEstimateZeroCrossHz( samples(), 256, 4096 )
ASSERT( hz > 820.0 and hz < 940.0 )

SfxTestDeleteFile( WAV_FILE )

'' end of sample-pitch.bas
