' TEST_MODE : MULTI_MODULE_OK

#include once "sfx_test_common.bi"

const MAX_FRAMES = 240000
const VOICE_COUNT = 64
const CHANNEL_COUNT = 16
const BURSTS = 4
const ANALYSIS_FRAMES = 12000

redim as single samples( 0 to ANALYSIS_FRAMES - 1 )

dim as integer burst
dim as integer voice_index
dim as integer channel_index
dim as string temp_dir
dim as string dump_file

'' Keep dump output in a stable writable directory rather than relying on CWD.
'' Some test launch paths execute from directories that do not match the source
'' tree, which causes the earlier relative path to miss the file.
temp_dir = trim( environ( "TMP" ) )
if( temp_dir = "" ) then
    temp_dir = trim( environ( "TEMP" ) )
end if

if( temp_dir = "" ) then
    temp_dir = trim( environ( "TMPDIR" ) )
end if

if( temp_dir = "" ) then
	temp_dir = curdir()

	if( temp_dir = "" ) then
		temp_dir = "."
	end if
end if

if( right( temp_dir, 1 ) <> "/" andalso right( temp_dir, 1 ) <> "\\" ) then
    temp_dir += "/"
end if

dump_file = temp_dir + "play-64-voice-smoke-driver.tmp"

SfxTestUseNullDriver()
SfxTestSetDriverDump( dump_file, MAX_FRAMES )

'' Start many short background phrases using PLAY in a rotating channel set.
'' This drives both the per-channel background queue and the global voice allocator.
for burst = 0 to BURSTS - 1
	for voice_index = 0 to VOICE_COUNT - 1
		channel_index = voice_index mod CHANNEL_COUNT
		play channel_index, "MB T240 L16 O4 C"
	next

	'' Keep advancing the mixer so queued PLAY calls decay and can be reused.
	fb_sfxUpdate( 9000 )
next

'' Let the run finish and make sure the mixer can still advance for a while
'' after all bursts have been queued.
fb_sfxUpdate( 60000 )

dim as integer total_frames
dim as integer frames = _
	SfxTestLoadDumpPrefix( dump_file, samples(), total_frames )
dim as zstring * 5 null_name = "null"
dim as FB_SFX_DRIVER_STATS stats

ASSERT( frames >= ANALYSIS_FRAMES )
ASSERT( total_frames >= 60000 )
ASSERT( fb_sfxDriverStatsSnapshot( @null_name, @stats ) = 0 )
ASSERT( stats.write_calls > 0 )
ASSERT( stats.frames_requested > 0 )
ASSERT( stats.frames_accepted = stats.frames_requested )
ASSERT( stats.frames_accepted >= 60000 )
ASSERT( stats.frames_dropped = 0 )
ASSERT( stats.short_writes = 0 )
ASSERT( stats.zero_writes = 0 )
ASSERT( stats.errors = 0 )

'' The early output should contain noticeable activity; lockups or drops usually
'' look like long quiet stretches in this smoke test.
ASSERT( SfxTestRms( samples(), 0, ANALYSIS_FRAMES ) > 0.002 )
ASSERT( SfxTestCountChanges( samples(), 0, ANALYSIS_FRAMES, 0.001 ) > 100 )

'' end of play-64-voice-smoke.bas
