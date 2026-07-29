''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: android-pset-throughput-smoke.bas
''
'' Purpose:
''
''     Isolate the Android GLES PSET batch path from slower command families.
''
'' Responsibilities:
''
''     - submit a game-sized run of opaque PSET commands
''     - force an ordered POINT readback after the batch
''     - report one timing result through the Android program log
''
'' This file intentionally does NOT contain:
''
''     - PAINT, text, image, or presentation timing
''     - a device-specific timing threshold
''     - a CPU rasterization fallback
''

#define __FB_GFXLIB3__
#include once "fbgfx.bi"

const screen_width = 320
const screen_height = 240
const point_count = 20000
dim as double started
dim as double elapsed
dim as integer ordered_pixel

if screenres( screen_width, screen_height, 32, 1, 0 ) <> 0 then end 1

started = timer
for index as integer = 0 to point_count - 1
	pset ( ( index * 37 ) mod screen_width, _
		( index * 53 ) mod screen_height ), _
		rgb( index and 255, ( index * 5 ) and 255, ( index * 11 ) and 255 )
next
ordered_pixel = point( 0, 0 )
elapsed = timer - started

screen 0
print "android_pset_throughput_seconds="; elapsed
print "android_pset_throughput_pixel="; ordered_pixel
end 0

'' end of android-pset-throughput-smoke.bas
