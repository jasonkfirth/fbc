''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: depth-normalization-smoke.bas
''
'' Purpose:
''
''     Verify gfxlib2-compatible normalization of legacy 15-bit and 24-bit
''     SCREENRES requests to the GPU formats that gfxlib3 actually owns.
''
'' Responsibilities:
''
''     - open 15-bit and 24-bit GFX_NULL modes
''     - verify SCREENINFO reports 16-bit and 32-bit storage respectively
''     - prove point writes and reads use the normalized formats
''
'' This file intentionally does NOT contain:
''
''     - platform display-depth enumeration
''     - visible-window presentation checks
''     - colour conversion tests beyond the two legacy aliases
''

#include once "fbgfx.bi"

dim mode_width as integer
dim mode_height as integer
dim mode_depth as integer
dim bytes_per_pixel as integer
dim pitch as integer
dim depth_15_color as uinteger
dim depth_24_color as uinteger
dim requested as string = lcase( command( 1 ) )
dim flags as integer

select case requested
case "", "automatic"
	flags = 0
case "null"
	flags = fb.GFX_NULL
case "opengl"
	flags = fb.GFX_OPENGL
case "vulkan"
	'' GFX_VULKAN is intentionally a gfxlib3 extension and is not declared by
	'' older gfxlib2 headers used for the compatibility baseline.
	#ifdef __FB_GFXLIB3__
	flags = fb.GFX_VULKAN
	#else
	end 9
	#endif
case else
	end 9
end select

if screenres( 17, 11, 15, 1, flags ) <> 0 then end 1
screeninfo mode_width, mode_height, mode_depth, bytes_per_pixel, pitch
if ( mode_width <> 17 ) or ( mode_height <> 11 ) then end 2
if ( mode_depth <> 16 ) or ( bytes_per_pixel <> 2 ) then end 3
pset ( 4, 5 ), rgb( 255, 0, 0 )
depth_15_color = point( 4, 5 )
screen 0
if depth_15_color <> &h00ff0000u then end 4

if screenres( 17, 11, 24, 1, flags ) <> 0 then end 5
screeninfo mode_width, mode_height, mode_depth, bytes_per_pixel, pitch
if ( mode_width <> 17 ) or ( mode_height <> 11 ) then end 6
if ( mode_depth <> 32 ) or ( bytes_per_pixel <> 4 ) then end 7
pset ( 4, 5 ), &h00112233u
depth_24_color = point( 4, 5 )
screen 0
if depth_24_color <> &h00112233u then end 8

print "GFX_DEPTH_NORMALIZATION_PASS"
end 0

'' end of depth-normalization-smoke.bas
