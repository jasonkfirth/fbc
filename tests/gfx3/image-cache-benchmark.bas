''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: image-cache-benchmark.bas
''
'' Purpose:
''
''     Separate CPU FB.IMAGE cache creation from the steady-state PSET PUT
''     workload.  This identifies whether an apparent sprite throughput cost is
''     GPU rendering work or one-time compatibility resource preparation.
''
'' Responsibilities:
''
''     - measure one cold CPU-image PSET PUT to ordered completion
''     - measure a later 4,096-sprite PSET run using the same image
''     - retain a final readback so neither result measures queue submission
''
'' This file intentionally does NOT contain:
''
''     - destination-reading PUT modes
''     - presentation timing or vendor-specific thresholds
''

#ifdef __FB_ANDROID__
	#ifndef __FB_GFXLIB3__
		#define __FB_GFXLIB3__
	#endif
#endif
#include once "fbgfx.bi"

#ifdef GFX3_OPENGL_TEST
	const backend_flags = fb.GFX_OPENGL
#elseif defined( GFX3_VULKAN_TEST )
	const backend_flags = fb.GFX_VULKAN
#else
	const backend_flags = 0
#endif

#ifdef __FB_ANDROID__
	const screen_width = 320
	const screen_height = 240
	const transfer_count = 512
#else
	const screen_width = 1024
	const screen_height = 768
	const transfer_count = 4096
#endif

dim as any ptr sprite_image
dim as any ptr second_image
dim as double started
dim as double cold_seconds, second_cold_seconds, hot_seconds
dim as integer final_pixel

if screenres( screen_width, screen_height, 32, 1, backend_flags ) <> 0 then end 1

sprite_image = imagecreate( 32, 32, rgba( 80, 150, 220, 128 ), 32 )
if sprite_image = 0 then end 2
line sprite_image, ( 0, 0 )-( 31, 31 ), rgba( 255, 0, 255, 255 ), b
line sprite_image, ( 4, 4 )-( 27, 27 ), rgba( 40, 210, 90, 180 ), bf

line ( 0, 0 )-( screen_width - 1, screen_height - 1 ), rgba( 7, 13, 29, 255 ), bf
started = timer
put ( 0, 0 ), sprite_image, pset
final_pixel = point( 16, 16 )
cold_seconds = timer - started

second_image = imagecreate( 32, 32, rgba( 80, 150, 220, 128 ), 32 )
if second_image = 0 then end 3
line second_image, ( 0, 0 )-( 31, 31 ), rgba( 255, 0, 255, 255 ), b
line second_image, ( 4, 4 )-( 27, 27 ), rgba( 40, 210, 90, 180 ), bf
line ( 0, 0 )-( screen_width - 1, screen_height - 1 ), rgba( 7, 13, 29, 255 ), bf
started = timer
put ( 0, 0 ), second_image, pset
final_pixel = point( 16, 16 )
second_cold_seconds = timer - started

line ( 0, 0 )-( screen_width - 1, screen_height - 1 ), rgba( 7, 13, 29, 255 ), bf
started = timer
for index as integer = 0 to transfer_count - 1
	put ( ( index * 17 ) mod ( screen_width - 32 ), _
		( index * 29 ) mod ( screen_height - 32 ) ), sprite_image, pset
next
final_pixel = point( 16, 16 )
hot_seconds = timer - started

imagedestroy sprite_image
imagedestroy second_image
screen 0
print "image_cache_benchmark_cold_seconds="; cold_seconds
print "image_cache_benchmark_second_cold_seconds="; second_cold_seconds
print "image_cache_benchmark_hot_seconds="; hot_seconds
print "image_cache_benchmark_pixel="; final_pixel
end 0

'' end of image-cache-benchmark.bas
