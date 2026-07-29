''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: line-pattern-smoke.bas
''
'' Purpose:
''
''     Diagnose the public styled LINE bit sequence against gfxlib2 semantics.
''
'' Responsibilities:
''
''     - draw one horizontal styled line through the public ABI
''     - fail at the first unexpected style bit
''
'' This file intentionally does NOT contain:
''
''     - clipping, diagonal, box, or presentation checks
''

#define __FB_GFXLIB3__
#include once "fbgfx.bi"

if screenres( 64, 8, 32, , fb.GFX_NULL ) <> 0 then end 1

dim as ulong style = &b1000000011110000
dim as ulong foreground = rgb( 0, 255, 0 )
line (0, 0)-(63, 0), foreground, , style

for x as integer = 0 to 63
	dim as ulong expected = rgb( 0, 0, 0 )
	if (style and (&h8000ul shr (x mod 16))) <> 0 then expected = foreground
	if point( x, 0 ) <> expected then
		print "styled LINE mismatch at x="; x; ", expected &h"; _
			hex( expected ); ", got &h"; hex( point( x, 0 ) )
		end 2
	end if
next

screen 0
end 0

'' end of line-pattern-smoke.bas
