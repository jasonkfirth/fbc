''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: presentation-smoke.bas
''
'' Purpose:
''
''     Exercise queued OpenGL presentation for each required logical color
''     representation.
''
'' Responsibilities:
''
''     - open visible 8, 16, and 32-bit OpenGL modes
''     - force an explicit synchronized PRESENT through SCREENSYNC
''     - prove presentation does not disturb exact logical pixel storage
''
'' This file intentionally does NOT contain:
''
''     - human screenshot comparison
''     - resize or fullscreen handling
''     - input event checks
''

#define __FB_GFXLIB3__
#include once "fbgfx.bi"
#ifdef __FB_WIN32__
	#include once "windows.bi"
#endif

dim as integer depths(0 to 2) = { 8, 16, 32 }

for depth_index as integer = 0 to ubound( depths )
	dim as integer depth = depths( depth_index )
	if screenres( 96, 64, depth, 2, 0 ) <> 0 then end 1
	if depth = 8 then
		palette 1, 255, 0, 0
		pset (12, 12), 1
		if point( 12, 12 ) <> 1 then end 2
	elseif depth = 16 then
		pset (12, 12), rgb( 255, 0, 0 )
		'' gfxlib2 expands RGB565 to 24-bit RGB without adding alpha.
		if point( 12, 12 ) <> (rgb( 255, 0, 0 ) and &h00FFFFFF) then end 3
	else
		pset (12, 12), rgb( 255, 0, 0 )
		if point( 12, 12 ) <> rgb( 255, 0, 0 ) then end 3
	end if
	line (20, 8)-(80, 48), rgb( 0, 255, 0 ), bf
	#ifdef __FB_WIN32__
		if depth = 8 then
			const native_title = "gfxlib3 presentation smoke"
			windowtitle native_title
			screensync
			if FindWindow( 0, strptr( native_title ) ) = 0 then end 4
		end if
	#endif
	screensync
	screenset 1, 1
	pset (4, 4), rgb( 0, 0, 255 )
	screensync
	screen 0
next

end 0

'' end of presentation-smoke.bas
