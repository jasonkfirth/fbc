''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: put-clipping-smoke.bas
''
'' Purpose:
''
''     Verify that GPU-target PUT clipping preserves source coordinates at
''     every screen and VIEW edge.
''
'' Responsibilities:
''
''     - exercise partial sprites at all four screen corners
''     - exercise a partial sprite against a smaller VIEW SCREEN rectangle
''     - verify a fully offscreen integer-limit sentinel remains harmless
''
'' This file intentionally does NOT contain:
''
''     - scaling, rotation, or projective transforms
''     - custom CPU blenders
''     - performance measurements
''

#include once "fbgfx.bi"

#ifdef GFX3_VULKAN_TEST
	const backend_flags = fb.GFX_VULKAN
#elseif defined( GFX3_OPENGL_TEST )
	const backend_flags = fb.GFX_OPENGL
#else
	const backend_flags = 0
#endif

if screenres( 16, 12, 32, 1, backend_flags ) <> 0 then end 1

dim as ulong background = rgb( 9, 19, 29 )
dim as ulong top_left = rgb( 210, 20, 30 )
dim as ulong top_right = rgb( 40, 220, 50 )
dim as ulong bottom_left = rgb( 60, 70, 230 )
dim as ulong bottom_right = rgb( 240, 200, 80 )
dim as any ptr sprite = imagecreate( 4, 4, rgb( 255, 0, 255 ), 32 )

if sprite = 0 then end 2
pset sprite, ( 0, 0 ), top_left
pset sprite, ( 3, 0 ), top_right
pset sprite, ( 0, 3 ), bottom_left
pset sprite, ( 3, 3 ), bottom_right

line ( 0, 0 )-( 15, 11 ), background, bf
put ( -3, -3 ), sprite, trans
put ( 15, -3 ), sprite, trans
put ( -3, 11 ), sprite, trans
put ( 15, 11 ), sprite, trans

dim as ulong actual = culng( point( 0, 0 ) )
if ( actual and &hFFFFFF ) <> ( bottom_right and &hFFFFFF ) then
	screen 0
	print "screen top-left expected=&h"; hex( bottom_right ); _
		" actual=&h"; hex( actual )
	end 3
end if
if ( culng( point( 15, 0 ) ) and &hFFFFFF ) <> _
	( bottom_left and &hFFFFFF ) then
	print "screen top-right expected=&h"; hex( bottom_left ); _
		" actual=&h"; hex( culng( point( 15, 0 ) ) )
	end 4
end if
if ( culng( point( 0, 11 ) ) and &hFFFFFF ) <> _
	( top_right and &hFFFFFF ) then
	print "screen bottom-left expected=&h"; hex( top_right ); _
		" actual=&h"; hex( culng( point( 0, 11 ) ) )
	end 5
end if
if ( culng( point( 15, 11 ) ) and &hFFFFFF ) <> _
	( top_left and &hFFFFFF ) then
	print "screen bottom-right expected=&h"; hex( top_left ); _
		" actual=&h"; hex( culng( point( 15, 11 ) ) )
	end 6
end if
if ( culng( point( 1, 1 ) ) and &hFFFFFF ) <> _
	( background and &hFFFFFF ) then
	print "screen background expected=&h"; hex( background ); _
		" actual=&h"; hex( culng( point( 1, 1 ) ) )
	end 7
end if

view screen ( 4, 3 )-( 11, 8 )
put ( 1, 0 ), sprite, trans
view screen ( 0, 0 )-( 15, 11 )
actual = culng( point( 4, 3 ) )
if ( actual and &hFFFFFF ) <> ( bottom_right and &hFFFFFF ) then
	screen 0
	print "VIEW corner expected=&h"; hex( bottom_right ); _
		" actual=&h"; hex( actual )
	end 8
end if
if ( culng( point( 3, 3 ) ) and &hFFFFFF ) <> _
	( background and &hFFFFFF ) then
	print "VIEW outside expected=&h"; hex( background ); _
		" actual=&h"; hex( culng( point( 3, 3 ) ) )
	end 9
end if

'' Sprite engines use this value for inactive objects. It must be rejected as
'' a whole command without overflowing destination-bound calculations.
put ( 0, csng( -2147483648 ) ), sprite, trans
if ( culng( point( 0, 0 ) ) and &hFFFFFF ) <> _
	( bottom_right and &hFFFFFF ) then end 10

imagedestroy sprite
screen 0
end 0

'' end of put-clipping-smoke.bas
