'' FreeBASIC classic AmigaOS graphics qualification
'' ----------------------------------------------
'' File: graphics.bas
'' Purpose: Verify native pixels and input across framebuffer formats.
'' Responsibilities: Draw colour bands, inspect the display, and exercise keys.
'' This file intentionally does NOT contain display or message implementation.

#include once "fbgfx.bi"

extern "C"
	declare function amiga_test_display_pixel(byval x as long, byval y as long) as ulong
	declare function amiga_test_input(byval kind as long, byval code as long, _
		byval x as long, byval y as long) as long
end extern

dim depths(0 to 2) as integer => {8, 16, 32}
screencontrol fb.SET_DRIVER_NAME, "AMIGA"
for mode as integer = 0 to 2
	if screenres(319, 200, depths(mode)) <> 0 then end 1
	screenlock
	for stripe as integer = 0 to 7
		dim red as integer = (stripe and 1) * 255
		dim green as integer = ((stripe shr 1) and 1) * 255
		dim blue as integer = ((stripe shr 2) and 1) * 255
		dim colour as ulong = rgb(red, green, blue)
		if depths(mode) = 8 then
			colour = stripe + 1
			palette colour, red, green, blue
		end if
		line (stripe * 40, 0)-((stripe + 1) * 40 - 1, 199), colour, bf
	next
	screenunlock
	for stripe as integer = 0 to 7
		dim expected as ulong = rgb((stripe and 1) * 255, _
			((stripe shr 1) and 1) * 255, ((stripe shr 2) and 1) * 255) and &hffffff
		if amiga_test_display_pixel(stripe * 40 + 10, 100) <> expected then end 2
	next
	if amiga_test_display_pixel(318, 199) <> &hffffff then end 3

	'' 0x20 is native raw A; bit 7 marks its release.
	if amiga_test_input(0, &h20, 0, 0) <> 0 then end 4
	if not multikey(fb.SC_A) then end 5
	if inkey <> "a" then end 6
	if amiga_test_input(0, &ha0, 0, 0) <> 0 then end 7
	if multikey(fb.SC_A) then end 8
	if amiga_test_input(1, 0, 73, 91) <> 0 then end 9
	dim x as integer, y as integer
	if getmouse(x, y) <> 0 then end 10
	if x <> 73 or y <> 91 then end 11
	screen 0
	print "Native graphics pixels and input passed at depth "; depths(mode)
next
end 0

'' end of graphics.bas
