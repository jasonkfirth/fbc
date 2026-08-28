''
'' FreeBASIC Sound Library benchmark
'' ---------------------------------
''
'' File: pcm-conversion-benchmark.bas
''
'' Purpose:
''
''     Measure the PCM conversion routines used at sfxlib driver boundaries.
''
'' Responsibilities:
''
''     - benchmark float-to-s16 playback conversion
''     - benchmark float-to-s32 playback conversion
''     - benchmark s16-to-float capture and decode conversion
''     - report conversion cost at the normal 48 kHz stereo block size
''     - retain deterministic checksums for old and new library comparisons
''
'' This file intentionally does NOT contain:
''
''     - mixer, oscillator, envelope, or effect timing
''     - audio-device access or real-time sleeps
''     - performance pass or fail thresholds
''

#inclib "sfx"

declare sub fb_sfxConvertFloatToS16 cdecl alias "fb_sfxConvertFloatToS16" _
	( _
		byval source as single ptr, _
		byval destination as short ptr, _
		byval samples as long _
	)

declare sub fb_sfxConvertFloatToS32 cdecl alias "fb_sfxConvertFloatToS32" _
	( _
		byval source as single ptr, _
		byval destination as long ptr, _
		byval samples as long _
	)

declare sub fb_sfxConvertS16ToFloat cdecl alias "fb_sfxConvertS16ToFloat" _
	( _
		byval source as short ptr, _
		byval destination as single ptr, _
		byval samples as long _
	)

const CHANNEL_COUNT = 2
const SAMPLE_RATE = 48000
const FRAMES_PER_BLOCK = 512
const SAMPLES_PER_BLOCK = FRAMES_PER_BLOCK * CHANNEL_COUNT
const DEFAULT_BLOCK_COUNT = 250000
const MAX_BLOCK_COUNT = 10000000
const WARM_BLOCK_COUNT = 4096

'' --------------------------------------------------------------------------
'' Timing and reporting helpers
'' --------------------------------------------------------------------------

private function ElapsedSeconds( byval startedAt as double ) as double
	dim as double elapsed = timer - startedAt

	'' TIMER wraps at midnight on platforms where it is time-of-day based.
	if( elapsed < 0.0 ) then elapsed += 86400.0
	if( elapsed <= 0.0 ) then elapsed = 0.000001
	return elapsed
end function

private sub PrintResult _
	( _
		byref conversionName as const string, _
		byval elapsed as double, _
		byval blockCount as integer, _
		byval checksum as ulong _
	)

	dim as double convertedSamples = cdbl( SAMPLES_PER_BLOCK ) * blockCount
	dim as double secondsPerBlock = elapsed / blockCount
	dim as double blocksPerAudioSecond = cdbl( SAMPLE_RATE ) / FRAMES_PER_BLOCK
	dim as double realTimeCpuPercent = secondsPerBlock * blocksPerAudioSecond * 100.0

	print "Conversion " & conversionName & _
		" seconds " & str( elapsed ) & _
		" MSamples/s " & str( convertedSamples / elapsed / 1000000.0 ) & _
		" microseconds/block " & str( secondsPerBlock * 1000000.0 ) & _
		" 48kHz-stereo-CPU-percent " & str( realTimeCpuPercent ) & _
		" checksum 0x" & hex( checksum, 8 )
end sub

'' --------------------------------------------------------------------------
'' Deterministic sample buffers
'' --------------------------------------------------------------------------

dim as integer blockCount = DEFAULT_BLOCK_COUNT
dim as single floatSource( 0 to SAMPLES_PER_BLOCK - 1 )
dim as short s16Source( 0 to SAMPLES_PER_BLOCK - 1 )
dim as short s16Destination( 0 to SAMPLES_PER_BLOCK - 1 )
dim as long s32Destination( 0 to SAMPLES_PER_BLOCK - 1 )
dim as single floatDestination( 0 to SAMPLES_PER_BLOCK - 1 )
dim as integer blockIndex
dim as double startedAt
dim as double floatToS16Seconds
dim as double floatToS32Seconds
dim as double s16ToFloatSeconds
dim as ulong floatToS16Checksum
dim as ulong floatToS32Checksum
dim as ulong s16ToFloatChecksum

if( len( command( 1 ) ) > 0 ) then
	blockCount = valint( command( 1 ) )
	if( blockCount < 1 orelse blockCount > MAX_BLOCK_COUNT ) then
		print "Block count must be between 1 and"; MAX_BLOCK_COUNT; "."
		end 2
	end if
end if

if( sizeof( single ) <> 4 orelse sizeof( short ) <> 2 orelse sizeof( long ) <> 4 ) then
	print "The PCM benchmark requires 32-bit SINGLE/LONG and 16-bit SHORT values."
	end 1
end if

for sampleIndex as integer = 0 to SAMPLES_PER_BLOCK - 1
	dim as integer floatPattern = ( ( sampleIndex * 7919 ) mod 10241 ) - 5120
	dim as integer s16Pattern = ( sampleIndex * 25173 + 13849 ) and &hFFFF

	'' A range of roughly -1.25 to +1.25 includes ordinary and clipped mixes.
	floatSource( sampleIndex ) = csng( floatPattern ) / 4096.0f
	s16Source( sampleIndex ) = cshort( s16Pattern - 32768 )
next sampleIndex

floatSource( 0 ) = -1.25f
floatSource( 1 ) = -1.0f
floatSource( 2 ) = -0.5f
floatSource( 3 ) = 0.0f
floatSource( 4 ) = 0.5f
floatSource( 5 ) = 1.0f
floatSource( 6 ) = 1.25f

'' --------------------------------------------------------------------------
'' Warm-up
'' --------------------------------------------------------------------------

for blockIndex = 1 to WARM_BLOCK_COUNT
	fb_sfxConvertFloatToS16( @floatSource( 0 ), @s16Destination( 0 ), SAMPLES_PER_BLOCK )
	fb_sfxConvertFloatToS32( @floatSource( 0 ), @s32Destination( 0 ), SAMPLES_PER_BLOCK )
	fb_sfxConvertS16ToFloat( @s16Source( 0 ), @floatDestination( 0 ), SAMPLES_PER_BLOCK )
next blockIndex

'' --------------------------------------------------------------------------
'' Timed driver conversions
'' --------------------------------------------------------------------------

startedAt = timer
for blockIndex = 1 to blockCount
	fb_sfxConvertFloatToS16( @floatSource( 0 ), @s16Destination( 0 ), SAMPLES_PER_BLOCK )
next blockIndex
floatToS16Seconds = ElapsedSeconds( startedAt )

startedAt = timer
for blockIndex = 1 to blockCount
	fb_sfxConvertFloatToS32( @floatSource( 0 ), @s32Destination( 0 ), SAMPLES_PER_BLOCK )
next blockIndex
floatToS32Seconds = ElapsedSeconds( startedAt )

startedAt = timer
for blockIndex = 1 to blockCount
	fb_sfxConvertS16ToFloat( @s16Source( 0 ), @floatDestination( 0 ), SAMPLES_PER_BLOCK )
next blockIndex
s16ToFloatSeconds = ElapsedSeconds( startedAt )

'' --------------------------------------------------------------------------
'' Output observation and results
'' --------------------------------------------------------------------------

floatToS16Checksum = culng( cushort( s16Destination( 0 ) ) )
floatToS16Checksum xor= culng( cushort( s16Destination( 17 ) ) ) shl 3
floatToS16Checksum xor= culng( cushort( s16Destination( 511 ) ) ) shl 7
floatToS16Checksum xor= culng( cushort( s16Destination( 1023 ) ) ) shl 11

floatToS32Checksum = culng( s32Destination( 0 ) )
floatToS32Checksum xor= culng( s32Destination( 17 ) ) shl 3
floatToS32Checksum xor= culng( s32Destination( 511 ) ) shl 7
floatToS32Checksum xor= culng( s32Destination( 1023 ) ) shl 11

dim as ulong ptr floatBits = cptr( ulong ptr, @floatDestination( 0 ) )
if( floatBits = 0 ) then
	print "The PCM benchmark could not observe its float output."
	end 1
end if

s16ToFloatChecksum = floatBits[0]
s16ToFloatChecksum xor= floatBits[17] shl 3
s16ToFloatChecksum xor= floatBits[511] shl 7
s16ToFloatChecksum xor= floatBits[1023] shl 11

print "sfxlib PCM conversion benchmark"
print "Block: "; FRAMES_PER_BLOCK; " stereo frames, "; SAMPLES_PER_BLOCK; " samples"
print "Blocks per conversion:"; blockCount
PrintResult "float-to-s16", floatToS16Seconds, blockCount, floatToS16Checksum
PrintResult "float-to-s32", floatToS32Seconds, blockCount, floatToS32Checksum
PrintResult "s16-to-float", s16ToFloatSeconds, blockCount, s16ToFloatChecksum

end 0

'' end of pcm-conversion-benchmark.bas
