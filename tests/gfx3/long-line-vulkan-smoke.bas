''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: long-line-vulkan-smoke.bas
''
'' Purpose:
''
''     Verify that Vulkan rasterizes long compatibility lines through their
''     visible clipped pixels without losing the original style phase.
''
'' Responsibilities:
''
''     - cover horizontal and diagonal lines longer than the shader step bound
''     - cover a fully clipped long line in the same packed line command
''     - compare exact visible and skipped pixels through ordered readbacks
''
'' This file intentionally does NOT contain:
''
''     - throughput measurements
''     - line clipping or style implementations
''     - alternate renderer selection
''

#include once "fbgfx.bi"

const long_line_style = &h8000
dim as ulong foreground = rgb( 37, 193, 71 )
dim as ulong later_color = rgb( 18, 73, 201 )
dim as ulong background = rgb( 3, 5, 7 )

if screenres( 64, 64, 32, 1, fb.GFX_VULKAN ) <> 0 then end 1
view screen ( 0, 0 )-( 63, 63 )
line ( 0, 0 )-( 63, 63 ), background, bf
if ( culng( point( 1, 8 ) ) and &hFFFFFF ) <> _
	( background and &hFFFFFF ) then
	screen 0
	print "initial clear mismatch: "; hex( culng( point( 1, 8 ) ) )
	end 9
end if

'' The visible samples correspond to original step indices 1,000,000 and
'' 1,000,016. Repeating the line with another color checks FIFO overwrite order.
line ( 2, 40 )-( 18, 40 ), foreground, , long_line_style
line ( -1000000, 24 )-( 1000000, 24 ), foreground, , long_line_style
line ( -1000000, 24 )-( 1000000, 24 ), later_color, , long_line_style
line ( 0, 0 )-( 1000000, 1000000 ), foreground, , long_line_style
line ( -1000000, -1000000 )-( 1000000, -1000000 ), foreground, , long_line_style

if ( culng( point( 0, 24 ) ) and &hFFFFFF ) <> _
	( later_color and &hFFFFFF ) then
	screen 0
	print "horizontal long line order mismatch: "; hex( culng( point( 0, 24 ) ) )
	end 2
end if
if ( culng( point( 1, 24 ) ) and &hFFFFFF ) <> _
	( background and &hFFFFFF ) then
	screen 0
	print "horizontal long line style phase mismatch: "; hex( culng( point( 1, 24 ) ) )
	end 3
end if
if ( culng( point( 16, 24 ) ) and &hFFFFFF ) <> _
	( later_color and &hFFFFFF ) then
	screen 0
	print "horizontal long line repeat pixel is missing: "; hex( culng( point( 16, 24 ) ) )
	end 4
end if
if ( culng( point( 0, 0 ) ) and &hFFFFFF ) <> _
	( foreground and &hFFFFFF ) then screen 0: print "diagonal start pixel is missing": end 5
if ( culng( point( 1, 1 ) ) and &hFFFFFF ) <> _
	( background and &hFFFFFF ) then screen 0: print "diagonal style phase mismatch": end 6
if ( culng( point( 16, 16 ) ) and &hFFFFFF ) <> _
	( foreground and &hFFFFFF ) then screen 0: print "diagonal repeat pixel is missing": end 7
if ( culng( point( 17, 17 ) ) and &hFFFFFF ) <> _
	( background and &hFFFFFF ) then screen 0: print "diagonal style phase mismatch at next step": end 8
if ( culng( point( 2, 40 ) ) and &hFFFFFF ) <> _
	( foreground and &hFFFFFF ) then screen 0: print "short line was lost before the long-line fallback": end 10
if ( culng( point( 3, 40 ) ) and &hFFFFFF ) <> _
	( background and &hFFFFFF ) then screen 0: print "short line style phase mismatch": end 11
if ( culng( point( 18, 40 ) ) and &hFFFFFF ) <> _
	( foreground and &hFFFFFF ) then screen 0: print "short line style repeat is missing": end 12
if ( culng( point( 0, 63 ) ) and &hFFFFFF ) <> _
	( background and &hFFFFFF ) then screen 0: print "fully clipped long line changed the page": end 13

screen 0
end 0

'' end of long-line-vulkan-smoke.bas
