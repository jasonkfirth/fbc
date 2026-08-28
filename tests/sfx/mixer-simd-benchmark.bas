''
'' FreeBASIC Sound Library benchmark
'' ---------------------------------
''
'' File: mixer-simd-benchmark.bas
''
'' Purpose:
''
''     Measure the complete offline sfxlib mixing path under representative
''     voice, sample, software MIDI, and effect workloads.
''
'' Responsibilities:
''
''     - render fixed 512-frame blocks through the null audio driver
''     - cover low and high polyphony generated waveforms
''     - distinguish contiguous from fractionally stepped sample playback
''     - measure the software FM fallback and global echo effect
''     - report real-time capacity and one-core cost for old/new comparisons
''
'' This file intentionally does NOT contain:
''
''     - audio-device sleeps or platform latency measurements
''     - diagnostic sample dumps or benchmark-time file access
''     - vendor-specific performance thresholds
''

#inclib "sfx"

declare sub fb_sfxUpdate cdecl alias "fb_sfxUpdate" _
	( byval frames as long )

declare function fb_sfxVoiceActiveCount cdecl alias "fb_sfxVoiceActiveCount" _
	( ) as long

declare sub fb_sfxMixerStopAll cdecl alias "fb_sfxMixerStopAll" ( )

declare sub fb_sfxSoundQueue cdecl alias "fb_sfxSoundQueue" _
	( _
		byval channel as long, _
		byval frequency as long, _
		byval duration as single, _
		byval volume as single, _
		byval waveform as long, _
		byval startDelay as long _
	)

declare sub fb_sfxSfxUnload cdecl alias "fb_sfxSfxUnload" _
	( byval assetId as long )

#ifndef SFX_BENCHMARK_LEGACY
declare function fb_sfxEchoCmd cdecl alias "fb_sfxEchoCmd" _
	( _
		byval wet as single, _
		byval delaySeconds as single, _
		byval feedback as single _
	) as long

declare sub fb_sfxEchoReset cdecl alias "fb_sfxEchoReset" ( )

declare function fb_sfxMidiSoftwareOpen cdecl alias "fb_sfxMidiSoftwareOpen" _
	( ) as long

declare sub fb_sfxMidiSoftwareClose cdecl alias "fb_sfxMidiSoftwareClose" ( )

declare function fb_sfxMidiSoftwareSend cdecl alias "fb_sfxMidiSoftwareSend" _
	( _
		byval status as ubyte, _
		byval data1 as ubyte, _
		byval data2 as ubyte _
	) as long
#endif

const CHANNEL_COUNT = 16
const SAMPLE_RATE = 44100
const FRAMES_PER_BLOCK = 512
#ifdef __FB_ANDROID__
const DEFAULT_BLOCK_COUNT = 8192
#else
const DEFAULT_BLOCK_COUNT = 1024
#endif
const MAX_BLOCK_COUNT = 10000
const WARM_BLOCK_COUNT = 32
const SAMPLE_ASSET_ID = 1

const WAVE_SINE = 0
const WAVE_SQUARE = 1
const WAVE_TRIANGLE = 2
const WAVE_SAW = 3
const WAVE_NOISE = 4

'' --------------------------------------------------------------------------
'' Timing and reporting
'' --------------------------------------------------------------------------

private function ElapsedSeconds( byval startedAt as double ) as double
	dim as double elapsed = timer - startedAt

	'' TIMER may be time-of-day based and therefore wraps at midnight.
	if( elapsed < 0.0 ) then elapsed += 86400.0
	if( elapsed <= 0.0 ) then elapsed = 0.000001
	return elapsed
end function

private sub RunTimedCase _
	( _
		byref caseName as const string, _
		byval blockCount as integer, _
		byval expectedOrdinaryVoices as integer _
	)

	dim as integer blockIndex
	dim as double startedAt
	dim as double elapsed
	dim as double renderedSeconds
	dim as double realTimeMultiple
	dim as long activeVoices

	for blockIndex = 1 to WARM_BLOCK_COUNT
		fb_sfxUpdate( FRAMES_PER_BLOCK )
	next blockIndex

	startedAt = timer
	for blockIndex = 1 to blockCount
		fb_sfxUpdate( FRAMES_PER_BLOCK )
	next blockIndex
	elapsed = ElapsedSeconds( startedAt )

	activeVoices = fb_sfxVoiceActiveCount()
	if( expectedOrdinaryVoices >= 0 andalso _
		activeVoices <> expectedOrdinaryVoices ) then
		print "Case "; caseName; " expected"; expectedOrdinaryVoices; _
			" active voices but observed"; activeVoices; "."
		end 3
	end if

	renderedSeconds = cdbl( blockCount * FRAMES_PER_BLOCK ) / SAMPLE_RATE
	realTimeMultiple = renderedSeconds / elapsed

	print "Case " & caseName & _
		" seconds " & str( elapsed ) & _
		" Mframes/s " & str( _
			cdbl( blockCount * FRAMES_PER_BLOCK ) / elapsed / 1000000.0 ) & _
		" realtime-x " & str( realTimeMultiple ) & _
		" one-core-percent " & str( 100.0 / realTimeMultiple ) & _
		" active-voices " & str( activeVoices )
end sub

'' --------------------------------------------------------------------------
'' Workload setup
'' --------------------------------------------------------------------------

private sub StartGeneratedVoices _
	( _
		byval voiceCount as integer, _
		byval waveform as integer, _
		byval durationSeconds as single _
	)

	if( voiceCount < 1 orelse voiceCount > 64 ) then
		print "Generated voice count is outside the supported range."
		end 4
	end if

	for voiceIndex as integer = 0 to voiceCount - 1
		fb_sfxSoundQueue( _
			voiceIndex mod CHANNEL_COUNT, _
			110 + ( voiceIndex * 17 ), _
			durationSeconds, _
			0.25f, _
			waveform, _
			0 _
		)
	next voiceIndex

	if( fb_sfxVoiceActiveCount() <> voiceCount ) then
		print "Could not allocate the requested generated voices."
		end 5
	end if
end sub

private sub StartSampleVoices _
	( byval voiceCount as integer, byval pitch as single )

	if( voiceCount < 1 orelse voiceCount > 64 ) then
		print "Sample voice count is outside the supported range."
		end 6
	end if

	for voiceIndex as integer = 0 to voiceCount - 1
		sfx loop _
			voiceIndex mod CHANNEL_COUNT, _
			SAMPLE_ASSET_ID, _
			pitch
	next voiceIndex

	if( fb_sfxVoiceActiveCount() <> voiceCount ) then
		print "Could not allocate the requested sample voices."
		end 7
	end if
end sub

#ifndef SFX_BENCHMARK_LEGACY
private sub StartFmVoices( byval voiceCount as integer )
	if( voiceCount < 1 orelse voiceCount > 32 ) then
		print "FM voice count is outside the supported range."
		end 8
	end if

	if( fb_sfxMidiSoftwareOpen() <> 0 ) then
		print "Could not initialize the software FM synthesizer."
		end 9
	end if

	for voiceIndex as integer = 0 to voiceCount - 1
		'' Channel 10 is percussion and its short voices would expire during a
		'' long offline run.  Use all 15 melodic MIDI channels instead.
		dim as ubyte midiChannel = voiceIndex mod ( CHANNEL_COUNT - 1 )
		dim as ubyte midiNote = 36 + ( voiceIndex mod 48 )

		if( midiChannel >= 9 ) then midiChannel += 1

		if( fb_sfxMidiSoftwareSend( &h90 or midiChannel, midiNote, 100 ) <> 0 ) then
			print "Could not start an FM benchmark voice."
			end 10
		end if
	next voiceIndex
end sub
#endif

private sub BenchmarkGenerated _
	( _
		byref caseName as const string, _
		byval voiceCount as integer, _
		byval waveform as integer, _
		byval durationSeconds as single, _
		byval blockCount as integer _
	)

	StartGeneratedVoices( voiceCount, waveform, durationSeconds )
	RunTimedCase caseName, blockCount, voiceCount
	fb_sfxMixerStopAll()
end sub

private sub BenchmarkSamples _
	( _
		byref caseName as const string, _
		byval voiceCount as integer, _
		byval pitch as single, _
		byval blockCount as integer _
	)

	StartSampleVoices( voiceCount, pitch )
	RunTimedCase caseName, blockCount, voiceCount
	fb_sfxMixerStopAll()
end sub

#ifndef SFX_BENCHMARK_LEGACY
private sub BenchmarkFm _
	( _
		byref caseName as const string, _
		byval voiceCount as integer, _
		byval blockCount as integer _
	)

	StartFmVoices( voiceCount )
	RunTimedCase caseName, blockCount, 0
	fb_sfxMidiSoftwareClose()
end sub
#endif

'' --------------------------------------------------------------------------
'' Benchmark matrix
'' --------------------------------------------------------------------------

dim as integer blockCount = DEFAULT_BLOCK_COUNT
dim as string sampleFilename = "examples/sfxlib/media/buzzer.wav"
dim as single caseDuration

if( len( command( 1 ) ) > 0 ) then
	blockCount = valint( command( 1 ) )
	if( blockCount < 1 orelse blockCount > MAX_BLOCK_COUNT ) then
		print "Block count must be between 1 and"; MAX_BLOCK_COUNT; "."
		end 2
	end if
end if

if( len( command( 2 ) ) > 0 ) then sampleFilename = command( 2 )

dim as integer sampleFile = freefile()
if( open( sampleFilename for input as #sampleFile ) <> 0 ) then
	print "Could not open sample asset: "; sampleFilename
	end 1
end if
close #sampleFile

setenviron "SFXLIB_DRIVER=null"
setenviron "SFXLIB_MIXER_DEBUG="
setenviron "SFXLIB_MIXER_DUMP="
setenviron "SFXLIB_DRIVER_DUMP="

sfx load SAMPLE_ASSET_ID, sampleFilename

'' Leave enough time beyond warm-up and measurement that generated voices
'' cannot expire because of rounding at the final benchmark block.
caseDuration = csng( _
	cdbl( blockCount + WARM_BLOCK_COUNT ) * FRAMES_PER_BLOCK / SAMPLE_RATE + 1.0 _
)

print "sfxlib offline mixer SIMD benchmark"
print "Block: "; FRAMES_PER_BLOCK; " stereo frames"
print "Blocks per case:"; blockCount

fb_sfxMixerStopAll()
RunTimedCase "idle", blockCount, 0

BenchmarkGenerated "triangle-1", 1, WAVE_TRIANGLE, caseDuration, blockCount
BenchmarkGenerated "triangle-4", 4, WAVE_TRIANGLE, caseDuration, blockCount
BenchmarkGenerated "triangle-16", 16, WAVE_TRIANGLE, caseDuration, blockCount
BenchmarkGenerated "triangle-32", 32, WAVE_TRIANGLE, caseDuration, blockCount
BenchmarkGenerated "triangle-64", 64, WAVE_TRIANGLE, caseDuration, blockCount
BenchmarkGenerated "sine-32", 32, WAVE_SINE, caseDuration, blockCount
BenchmarkGenerated "square-32", 32, WAVE_SQUARE, caseDuration, blockCount
BenchmarkGenerated "saw-32", 32, WAVE_SAW, caseDuration, blockCount
BenchmarkGenerated "noise-32", 32, WAVE_NOISE, caseDuration, blockCount

'' The 11025 Hz fixture advances contiguously at pitch 4 on a 44100 Hz mix.
BenchmarkSamples "sample-1x-1", 1, 4.0f, blockCount
BenchmarkSamples "sample-1x-16", 16, 4.0f, blockCount
BenchmarkSamples "sample-1x-64", 64, 4.0f, blockCount
BenchmarkSamples "sample-pitched-16", 16, 5.0f, blockCount
BenchmarkSamples "sample-pitched-64", 64, 5.0f, blockCount

#ifndef SFX_BENCHMARK_LEGACY
	BenchmarkFm "fm-1", 1, blockCount
	BenchmarkFm "fm-16", 16, blockCount
	BenchmarkFm "fm-32", 32, blockCount

	StartGeneratedVoices( 32, WAVE_SAW, caseDuration )
	if( fb_sfxEchoCmd( 0.25f, 0.11f, 0.32f ) <> 0 ) then
		print "Could not enable the echo benchmark case."
		end 11
	end if
	RunTimedCase "saw-echo-32", blockCount, 32
	fb_sfxEchoReset()
	fb_sfxMixerStopAll()
#endif

fb_sfxSfxUnload( SAMPLE_ASSET_ID )
end 0

'' end of mixer-simd-benchmark.bas
