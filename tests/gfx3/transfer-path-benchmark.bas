''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: transfer-path-benchmark.bas
''
'' Purpose:
''
''     Isolate one large compatible PSET PUT batch from synchronous GET so a
''     renderer performance regression can be assigned to its real command
''     path rather than hidden by a combined transfer fixture.
''
'' Responsibilities:
''
''     - submit 4,096 public PSET PUT commands
''     - force completion using one ordered POINT readback
''     - select the normal, OpenGL, or Vulkan renderer without changing work
''
'' This file intentionally does NOT contain:
''
''     - the other built-in PUT modes
''     - a repeated GET loop
''     - renderer-private API calls
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
	const default_transfer_count = 512
#else
	const screen_width = 1024
	const screen_height = 768
	const default_transfer_count = 4096
#endif

dim as any ptr sprite_image
dim as integer final_pixel
dim as integer transfer_count = valint( command( 1 ) )
dim as double started

if transfer_count <= 0 then transfer_count = default_transfer_count

if screenres( screen_width, screen_height, 32, 1, backend_flags ) <> 0 then end 1

sprite_image = imagecreate( 32, 32, rgba( 80, 150, 220, 128 ), 32 )
if sprite_image = 0 then end 2
line sprite_image, ( 0, 0 )-( 31, 31 ), rgba( 255, 0, 255, 255 ), b
line sprite_image, ( 4, 4 )-( 27, 27 ), rgba( 40, 210, 90, 180 ), bf

line ( 0, 0 )-( screen_width - 1, screen_height - 1 ), rgba( 7, 13, 29, 255 ), bf
started = timer
for index as integer = 0 to transfer_count - 1
	#ifdef TRANS_MODE
		put ( ( index * 17 ) mod ( screen_width - 32 ), _
			( index * 29 ) mod ( screen_height - 32 ) ), sprite_image, trans
	#else
	put ( ( index * 17 ) mod ( screen_width - 32 ), _
		( index * 29 ) mod ( screen_height - 32 ) ), sprite_image, pset
	#endif
next
final_pixel = point( 16, 16 )

imagedestroy sprite_image
screen 0
print "transfer_path_pset_seconds="; timer - started
print "transfer_path_pixel="; final_pixel
end 0

'' end of transfer-path-benchmark.bas
