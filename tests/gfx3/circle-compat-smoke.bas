''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: circle-compat-smoke.bas
''
'' Purpose:
''
''     Compare representative CIRCLE, ellipse, filled, and arc pixels with
''     the existing gfxlib2 rasterization contract.
''
'' Responsibilities:
''
''     - exercise full circles, aspect-ratio ellipses, fills, and arcs
''     - hash every logical screen pixel after an opaque drawing fixture
''     - run unchanged source against gfxlib2 and gfxlib3 backends
''
'' This file intentionally does NOT contain:
''
''     - alpha primitive behavior, which has its own focused fixture
''     - GPU performance measurements
''     - custom DRAW or text coverage
''

#ifndef GFX2_REFERENCE
	#define __FB_GFXLIB3__
#endif
#include once "fbgfx.bi"

dim requested as string = lcase( command( 1 ) )
dim backend_flags as integer

select case requested
case "", "automatic"
	backend_flags = 0
case "null"
	backend_flags = fb.GFX_NULL
case "opengl"
	backend_flags = fb.GFX_OPENGL
#ifndef GFX2_REFERENCE
case "vulkan"
	backend_flags = fb.GFX_VULKAN
#endif
case else
	end 1
end select

if screenres( 96, 80, 32, 1, backend_flags ) <> 0 then end 2
line ( 0, 0 )-( 95, 79 ), rgb( 0, 0, 0 ), bf

circle ( 12, 14 ), 9, rgb( 255, 0, 0 )
circle ( 36, 14 ), 9, rgb( 0, 255, 0 ), 0, 6.283186, 0.5
circle ( 61, 14 ), 9, rgb( 0, 0, 255 ), 0, 6.283186, 2.0
circle ( 84, 14 ), 8, rgb( 255, 255, 0 ), 0.4, 2.5

circle ( 16, 47 ), 10, rgb( 255, 0, 255 ), , , , f
circle ( 46, 47 ), 10, rgb( 0, 255, 255 ), 0, 6.283186, 0.5, f
circle ( 76, 47 ), 10, rgb( 255, 128, 0 ), 3.8, 1.0, 1.5

dim as ulong hash = &h811C9DC5u
for y as integer = 0 to 79
	for x as integer = 0 to 95
		hash xor= culng( point( x, y ) )
		hash *= &h01000193u
	next
next

if hash <> &h6BDC39D7u then
	screen 0
	print "gfxlib circle fixture hash " & hex( hash, 8 )
	end 3
end if

screen 0
print "gfxlib circle fixture hash " & hex( hash, 8 )
end 0

'' end of circle-compat-smoke.bas
