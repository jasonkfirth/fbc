''
'' Project: FreeBASIC gfxlib3 benchmarks
'' --------------------------------------
''
'' File: locked-point-pset-benchmark.bas
''
'' Purpose:
''
''     Measure the legacy software-rendering pattern which reads and rewrites
''     individual screen pixels inside SCREENLOCK.
''
'' Responsibilities:
''
''     - run identical POINT/PSET work through gfxlib2 and gfxlib3
''     - begin each frame with a full CLS, matching software overlay renderers
''     - force the last frame to become observable before reporting time
''
'' This file intentionally does NOT contain:
''
''     - a custom font or game-specific drawing code
''     - a performance threshold
''     - gfxlib3 extension API calls
''

#ifdef GFX3_TEST
	#define __FB_GFXLIB3__
#endif
#include once "fbgfx.bi"

const screen_width = 320
const screen_height = 200
const frame_count = 48
const pixels_per_frame = 8192

#ifdef GFX3_VULKAN_TEST
	const backend_flags = fb.GFX_VULKAN
#elseif defined( GFX3_OPENGL_TEST )
	const backend_flags = fb.GFX_OPENGL
#else
	const backend_flags = 0
#endif

if screenres( screen_width, screen_height, 32, 2, backend_flags ) <> 0 then
	print "locked_point_pset_error=screenres"
	end 1
end if

dim as double started = timer
for frame as integer = 0 to frame_count - 1
	screenlock
	cls
	for pixel_index as integer = 0 to pixels_per_frame - 1
		dim as integer x = (pixel_index * 73 + frame * 11) mod screen_width
		dim as integer y = (pixel_index * 29 + frame * 7) mod screen_height
		dim as ulong destination = point( x, y )
		dim as ulong source = rgb( _
			(pixel_index * 3) and 255, _
			(pixel_index * 5) and 255, _
			(pixel_index * 7) and 255 )

		''
		'' A cheap integer mix keeps the benchmark focused on the graphics calls
		'' while retaining the destination dependency of a software alpha loop.
		''
		pset (x, y), (destination and &h00FEFEFEul) xor source
	next
	screenunlock
	screencopy
next
screensync
dim as double elapsed = timer - started
dim as ulong final_pixel = point( _
	((pixels_per_frame - 1) * 73 + (frame_count - 1) * 11) mod screen_width, _
	((pixels_per_frame - 1) * 29 + (frame_count - 1) * 7) mod screen_height )

screen 0
print "locked_point_pset_seconds="; elapsed
print "locked_point_pset_pairs="; frame_count * pixels_per_frame
print "locked_point_pset_pairs_per_second="; _
	(frame_count * pixels_per_frame) / elapsed
print "locked_point_pset_pixel="; final_pixel

end 0

'' end of locked-point-pset-benchmark.bas
