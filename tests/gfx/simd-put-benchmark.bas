''
'' Project: FreeBASIC gfxlib2 benchmarks
'' ------------------------------------
''
'' File: simd-put-benchmark.bas
''
'' Purpose:
''
''     Measure gfxlib2's CPU pixel-fill and PUT compositing kernels without
''     including display-driver or presentation time.
''
'' Responsibilities:
''
''     - render into offscreen 32-bit FB.IMAGE destinations
''     - exercise logical, transparent, alpha, blend, additive, and alpha-mask
''       operations over vector-sized rows
''     - report completed pixel throughput for scalar and SIMD comparisons
''     - retain output checksums so every measured result remains observable
''
'' This file intentionally does NOT contain:
''
''     - display refresh or GPU command measurements
''     - correctness reference implementations
''     - architecture-specific entry-point calls
''     - performance pass or fail thresholds
''

#include once "fbgfx.bi"

const IMAGE_WIDTH = 256
const IMAGE_HEIGHT = 256
const PIXELS_PER_OPERATION = IMAGE_WIDTH * IMAGE_HEIGHT
#ifdef __FB_ANDROID__
const DEFAULT_ITERATIONS = 8192
#else
const DEFAULT_ITERATIONS = 1024
#endif
const MAX_ITERATIONS = 100000
const WARM_ITERATIONS = 32
const RESULT_COUNT = 10

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

private sub PrintResult _
	( _
		byref operationName as const string, _
		byval elapsed as double, _
		byval iterations as integer, _
		byval checksum as ulong _
	)

	dim as double pixels = cdbl( PIXELS_PER_OPERATION ) * iterations

	print "Operation " & operationName & _
		" seconds " & str( elapsed ) & _
		" MPixels/s " & str( pixels / elapsed / 1000000.0 ) & _
		" checksum 0x" & hex( checksum, 8 )
end sub

'' --------------------------------------------------------------------------
'' Stable source and destination images
'' --------------------------------------------------------------------------

dim as integer iterations = DEFAULT_ITERATIONS
dim as any ptr sourceImage
dim as any ptr destinationImage
dim as any ptr maskImage
dim as ubyte ptr maskPixels
dim as integer maskPitch
dim as integer infoWidth, infoHeight, infoBpp, infoSize
dim as double startedAt
dim as double elapsed( 0 to RESULT_COUNT - 1 )
dim as ulong checksum( 0 to RESULT_COUNT - 1 )

if( len( command( 1 ) ) > 0 ) then
	iterations = valint( command( 1 ) )
	if( iterations < 1 orelse iterations > MAX_ITERATIONS ) then
		print "Iteration count must be between 1 and"; MAX_ITERATIONS; "."
		end 2
	end if
end if

'' The null driver supplies gfxlib2's CPU context without opening a display.
if( screenres( 64, 48, 32, 1, fb.GFX_NULL ) <> 0 ) then
	print "Could not initialize the gfxlib2 null driver."
	end 1
end if

sourceImage = imagecreate( IMAGE_WIDTH, IMAGE_HEIGHT, 0, 32 )
destinationImage = imagecreate( IMAGE_WIDTH, IMAGE_HEIGHT, 0, 32 )
maskImage = imagecreate( IMAGE_WIDTH, IMAGE_HEIGHT, 0, 8 )
if( sourceImage = 0 orelse destinationImage = 0 orelse maskImage = 0 ) then
	if( maskImage <> 0 ) then imagedestroy maskImage
	if( destinationImage <> 0 ) then imagedestroy destinationImage
	if( sourceImage <> 0 ) then imagedestroy sourceImage
	screen 0
	print "Could not allocate the benchmark images."
	end 3
end if

if( imageinfo( maskImage, infoWidth, infoHeight, infoBpp, maskPitch, _
	maskPixels, infoSize ) <> 0 ) then
	imagedestroy maskImage
	imagedestroy destinationImage
	imagedestroy sourceImage
	screen 0
	print "Could not inspect the alpha-mask image."
	end 4
end if

if( maskPixels = 0 orelse maskPitch < IMAGE_WIDTH ) then
	imagedestroy maskImage
	imagedestroy destinationImage
	imagedestroy sourceImage
	screen 0
	print "The alpha-mask image has an invalid memory layout."
	end 5
end if

for y as integer = 0 to IMAGE_HEIGHT - 1
	for x as integer = 0 to IMAGE_WIDTH - 1
		dim as ulong sourceColor = rgba( _
			( x * 37 + y * 11 ) and 255, _
			( x * 17 + y * 53 ) and 255, _
			( x * 73 + y * 29 ) and 255, _
			( x * 41 + y * 67 ) and 255 _
		)

		if( ( x + y ) mod 11 = 0 ) then
			sourceColor = rgba( 255, 0, 255, ( x * 41 + y * 67 ) and 255 )
		end if

		pset sourceImage, ( x, y ), sourceColor
		pset destinationImage, ( x, y ), rgba( _
			( x * 13 + y * 31 ) and 255, _
			( x * 61 + y * 7 ) and 255, _
			( x * 23 + y * 47 ) and 255, _
			( x * 19 + y * 89 ) and 255 _
		)
		maskPixels[y * maskPitch + x] = ( x * 29 + y * 71 + 3 ) and 255
	next x
next y

'' Warm every lazy PUT dispatch table outside the measured intervals.
for index as integer = 1 to WARM_ITERATIONS
	put destinationImage, ( 0, 0 ), sourceImage, and
	put destinationImage, ( 0, 0 ), sourceImage, or
	put destinationImage, ( 0, 0 ), sourceImage, xor
	put destinationImage, ( 0, 0 ), sourceImage, preset
	put destinationImage, ( 0, 0 ), sourceImage, trans
	put destinationImage, ( 0, 0 ), sourceImage, alpha
	put destinationImage, ( 0, 0 ), sourceImage, alpha, 137
	put destinationImage, ( 0, 0 ), sourceImage, add, 173
	put destinationImage, ( 0, 0 ), maskImage, alpha
next index

'' --------------------------------------------------------------------------
'' Timed CPU compositing operations
'' --------------------------------------------------------------------------

startedAt = timer
for index as integer = 1 to iterations
	line destinationImage, ( 0, 0 )-( IMAGE_WIDTH - 1, IMAGE_HEIGHT - 1 ), _
		rgba( 17, 43, 91, 157 ), bf
next index
elapsed( 0 ) = ElapsedSeconds( startedAt )
checksum( 0 ) = culng( point( 31, 47, destinationImage ) )

startedAt = timer
for index as integer = 1 to iterations
	put destinationImage, ( 0, 0 ), sourceImage, and
next index
elapsed( 1 ) = ElapsedSeconds( startedAt )
checksum( 1 ) = culng( point( 31, 47, destinationImage ) )

startedAt = timer
for index as integer = 1 to iterations
	put destinationImage, ( 0, 0 ), sourceImage, or
next index
elapsed( 2 ) = ElapsedSeconds( startedAt )
checksum( 2 ) = culng( point( 31, 47, destinationImage ) )

startedAt = timer
for index as integer = 1 to iterations
	put destinationImage, ( 0, 0 ), sourceImage, xor
next index
elapsed( 3 ) = ElapsedSeconds( startedAt )
checksum( 3 ) = culng( point( 31, 47, destinationImage ) )

startedAt = timer
for index as integer = 1 to iterations
	put destinationImage, ( 0, 0 ), sourceImage, preset
next index
elapsed( 4 ) = ElapsedSeconds( startedAt )
checksum( 4 ) = culng( point( 31, 47, destinationImage ) )

startedAt = timer
for index as integer = 1 to iterations
	put destinationImage, ( 0, 0 ), sourceImage, trans
next index
elapsed( 5 ) = ElapsedSeconds( startedAt )
checksum( 5 ) = culng( point( 31, 47, destinationImage ) )

startedAt = timer
for index as integer = 1 to iterations
	put destinationImage, ( 0, 0 ), sourceImage, alpha
next index
elapsed( 6 ) = ElapsedSeconds( startedAt )
checksum( 6 ) = culng( point( 31, 47, destinationImage ) )

startedAt = timer
for index as integer = 1 to iterations
	put destinationImage, ( 0, 0 ), sourceImage, alpha, 137
next index
elapsed( 7 ) = ElapsedSeconds( startedAt )
checksum( 7 ) = culng( point( 31, 47, destinationImage ) )

startedAt = timer
for index as integer = 1 to iterations
	put destinationImage, ( 0, 0 ), sourceImage, add, 173
next index
elapsed( 8 ) = ElapsedSeconds( startedAt )
checksum( 8 ) = culng( point( 31, 47, destinationImage ) )

startedAt = timer
for index as integer = 1 to iterations
	put destinationImage, ( 0, 0 ), maskImage, alpha
next index
elapsed( 9 ) = ElapsedSeconds( startedAt )
checksum( 9 ) = culng( point( 31, 47, destinationImage ) )

imagedestroy maskImage
imagedestroy destinationImage
imagedestroy sourceImage
screen 0

'' PRINT targets the graphics console while a screen mode is active.  Emit the
'' report after SCREEN 0 so Android logcat and redirected host output receive it.
print "gfxlib2 offscreen SIMD PUT benchmark"
print "Image: "; IMAGE_WIDTH; "x"; IMAGE_HEIGHT; ", iterations:"; iterations
PrintResult "FILL", elapsed( 0 ), iterations, checksum( 0 )
PrintResult "AND", elapsed( 1 ), iterations, checksum( 1 )
PrintResult "OR", elapsed( 2 ), iterations, checksum( 2 )
PrintResult "XOR", elapsed( 3 ), iterations, checksum( 3 )
PrintResult "PRESET", elapsed( 4 ), iterations, checksum( 4 )
PrintResult "TRANS", elapsed( 5 ), iterations, checksum( 5 )
PrintResult "ALPHA", elapsed( 6 ), iterations, checksum( 6 )
PrintResult "BLEND", elapsed( 7 ), iterations, checksum( 7 )
PrintResult "ADD", elapsed( 8 ), iterations, checksum( 8 )
PrintResult "ALPHA-MASK", elapsed( 9 ), iterations, checksum( 9 )
end 0

'' end of simd-put-benchmark.bas
