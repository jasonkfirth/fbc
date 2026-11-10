'' Project: FreeBASIC GFXLIB2 tests
'' File: vgaemu-roundtrip.bas
'' Purpose: Verify VGA palette ports against the indexed graphics palette.
'' Responsibilities: Test component writes, palette reads, and retrace status.
'' This file intentionally does NOT contain: Direct hardware I/O or VGA drivers.

'' TEST_MODE : COMPILE_AND_RUN_OK

#include once "fbgfx.bi"

if screenres( 64, 64, 8, 1, fb.GFX_NULL ) <> 0 then end 1

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

'' end of vgaemu-roundtrip.bas
