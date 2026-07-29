''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: image-allocation-benchmark.bas
''
'' Purpose:
''
''     Measure the compatibility-image allocation family shared by gfxlib2
''     and gfxlib3.
''
'' Responsibilities:
''
''     - exercise IMAGECREATE and IMAGEDESTROY repeatedly
''     - validate IMAGEINFO for each newly allocated image
''     - report a completed CPU allocation workload without graphics timing
''
'' This file intentionally does NOT contain:
''
''     - GPU drawing or presentation timing
''     - file image decoding
''
#include once "fbgfx.bi"

const allocation_count = 10000

dim as any ptr image
dim as double started
dim as double elapsed
dim as integer index
dim as integer image_width
dim as integer image_height
dim as integer image_bpp
dim as integer image_pitch
dim as any ptr image_pixels
dim as integer image_size
dim as integer checksum

if screenres( 320, 240, 32 ) <> 0 then end 1

started = timer

for index = 1 to allocation_count
    image = imagecreate( 64, 64, rgb( index and 255, 80, 160 ) )
    if image = 0 then end 2

    if imageinfo( image, image_width, image_height, image_bpp, image_pitch, _
                  image_pixels, image_size ) <> 0 then end 3
    if ( image_width <> 64 ) or ( image_height <> 64 ) or _
       ( image_bpp <> 4 ) or ( image_pixels = 0 ) then end 4

    checksum += image_width + image_height + image_bpp + image_pitch + image_size
    imagedestroy image
next

elapsed = timer - started

screen 0

print "image_allocation_benchmark_seconds="; elapsed
print "image_allocation_benchmark_count="; allocation_count
print "image_allocation_benchmark_checksum="; checksum

'' end of image-allocation-benchmark.bas
