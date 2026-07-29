''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: paint-pattern-gles-smoke.bas
''
'' Purpose:
''
''     Verify the bounded GLES GPU-pattern PAINT path independently of the
''     cross-backend depth fixture.
''
'' Responsibilities:
''
''     - select gfxlib3 through the public compiler option
''     - fill a clipped indexed target with an absolute-coordinate tile
''     - report the expected and observed tile value for physical diagnostics
''
'' This file intentionally does NOT contain:
''
''     - CPU fallback checks
''     - RGB565 or 32-bit pattern layouts
''     - performance measurements
''
#include once "fbgfx.bi"

function tile_index( byval x as integer, byval y as integer ) as ulong
	return culng( ((x and 7) * 19 + ((y and 7) * 7) + 3) and &hFF )
end function

dim as string pattern
dim as ulong actual_value
dim as ulong expected_value

for y as integer = 0 to 7
	for x as integer = 0 to 7
		pattern += chr( tile_index( x, y ) )
	next
next

if screenres( 20, 20, 8, 1, 0 ) <> 0 then end 10
view ( 3, 4 )-( 12, 13 )
line ( 0, 0 )-( 9, 9 ), rgb( 255, 255, 255 ), b
paint ( 1, 1 ), pattern, rgb( 255, 255, 255 )

actual_value = culng( point( 1, 1 ) )
expected_value = tile_index( 4, 5 )
print "GFX3_GLES_PATTERN expected=" & expected_value & _
	" actual=" & actual_value
screen 0

'' Return the observed byte plus one so Android's exit log remains useful even
'' when the short graphical program exits before console text is flushed.
if actual_value <> expected_value then end actual_value + 1
end 0

'' end of paint-pattern-gles-smoke.bas
