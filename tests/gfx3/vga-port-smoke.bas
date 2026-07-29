''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: vga-port-smoke.bas
''
'' Purpose:
''
''     Verify indexed-mode VGA DAC and status-port compatibility hooks.
''
'' Responsibilities:
''
''     - write a palette entry as sequential six-bit RGB components
''     - read the same entry through the emulated DAC read index
''     - check the synchronized vertical-retrace status value
''
'' This file intentionally does NOT contain:
''
''     - direct hardware I/O
''     - non-VGA registers
''     - true-color port fallback behavior
''

#define __FB_GFXLIB3__
#include once "fbgfx.bi"

#ifdef GFX3_OPENGL_TEST
	const backend_flags = 0
#else
	const backend_flags = fb.GFX_NULL
#endif

if screenres( 64, 64, 8, 1, backend_flags ) <> 0 then end 1

out &h3C8, 5
out &h3C9, 63
out &h3C9, 32
out &h3C9, 1

dim as integer red, green, blue
palette get 5, red, green, blue
if red <> 255 then end 2
if green <> 129 then end 3
if blue <> 4 then end 4

out &h3C7, 5
if inp( &h3C9 ) <> 63 then end 5
if inp( &h3C9 ) <> 32 then end 6
if inp( &h3C9 ) <> 1 then end 7
if inp( &h3DA ) <> 8 then end 8

screen 0
end 0

'' end of vga-port-smoke.bas
