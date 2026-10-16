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
	declare function amiga_test_multikey_queued(byval code as long, _
		byval scancode as long) as long
	declare function amiga_test_pointer_hidden() as long
	declare function amiga_test_display_mode(byref width as long, byref height as long, _
		byref mode_id as ulong) as long
end extern

private function NearestLevel(byval value as integer, levels() as integer) as integer
	dim best as integer = levels(0), distance as integer = abs(value - levels(0))
	for index as integer = 1 to ubound(levels)
		dim candidate as integer = abs(value - levels(index))
		if candidate < distance then
			distance = candidate
			best = levels(index)
		end if
	next
	return best
end function

dim depths(0 to 2) as integer => {8, 16, 32}
screencontrol fb.SET_DRIVER_NAME, "AMIGA"
for mode as integer = 0 to 2
	if screenres(319, 200, depths(mode), 2) <> 0 then end 1
	if amiga_test_pointer_hidden() <> 0 then end 14
	setmouse , , 0
	if amiga_test_pointer_hidden() <> 1 then end 15
	setmouse 20, 30
	if amiga_test_pointer_hidden() <> 1 then end 16
	setmouse , , 1
	if amiga_test_pointer_hidden() <> 0 then end 17
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

	'' Mixed channels expose byte-order errors that pure primaries can miss.
	'' AGA reduces true-colour frames to RGB332; indexed colours retain their
	'' requested components. A true-colour conversion must select the nearest
	'' available component, rather than merely discarding its low bits.
	dim levels3(0 to 7) as integer => {0, 36, 73, 109, 146, 182, 219, 255}
	dim levels2(0 to 3) as integer => {0, 85, 170, 255}
	for sample as integer = 0 to 255
		dim red as integer = (sample * 37 + 18) and 255
		dim green as integer = (sample * 73 + 52) and 255
		dim blue as integer = (sample * 109 + 86) and 255
		dim expected as ulong
		if depths(mode) = 8 then
			palette 17, red, green, blue
			pset (17, 17), 17
			expected = rgb(red, green, blue) and &hffffff
		else
			pset (17, 17), rgb(red, green, blue)
			if depths(mode) = 16 then
				red = ((red shr 3) * 255) \ 31
				green = ((green shr 2) * 255) \ 63
				blue = ((blue shr 3) * 255) \ 31
			end if
			expected = rgb(NearestLevel(red, levels3()), NearestLevel(green, levels3()), _
				NearestLevel(blue, levels2())) and &hffffff
		end if
		dim observed as ulong = amiga_test_display_pixel(17, 17)
		if observed <> expected then
			screen 0
			print "Mixed colour mismatch: depth="; depths(mode); " sample="; sample; _
				" expected="; hex(expected, 6); " observed="; hex(observed, 6)
			end 12
		end if
	next
	if depths(mode) = 8 then
		'' Changing a live palette must update pixels without another draw.
		palette 17, 85, 170, 255
		if amiga_test_display_pixel(17, 17) <> &h55aaff then end 13
	end if

	'' Hidden-page drawing, copying and page selection must use the visible
	'' framebuffer. Games commonly draw a whole menu before SCREENCOPY.
	dim blue_pen as ulong = rgb(0, 0, 255)
	dim green_pen as ulong = rgb(0, 255, 0)
	dim red_pen as ulong = rgb(255, 0, 0)
	if depths(mode) = 8 then
		palette 20, 0, 0, 255
		palette 21, 0, 255, 0
		palette 22, 255, 0, 0
		blue_pen = 20 : green_pen = 21 : red_pen = 22
	end if
	pset (24, 24), blue_pen
	screenset 1, 0
	pset (24, 24), green_pen
	if amiga_test_display_pixel(24, 24) <> &h0000ff then end 18
	screencopy
	if amiga_test_display_pixel(24, 24) <> &h00ff00 then end 19
	screenset 0, 1
	pset (24, 24), red_pen
	if amiga_test_display_pixel(24, 24) <> &h00ff00 then end 20
	screenset 0, 0
	if amiga_test_display_pixel(24, 24) <> &hff0000 then end 21

	'' 0x20 is native raw A; bit 7 marks its release.
	if amiga_test_input(0, &h20, 0, 0) <> 0 then end 4
	if not multikey(fb.SC_A) then end 5
	if inkey <> "a" then end 6
	if amiga_test_input(0, &ha0, 0, 0) <> 0 then end 7
	if multikey(fb.SC_A) then end 8
	'' The helper queues Escape without polling it before the actual query.
	if amiga_test_multikey_queued(&h45, fb.SC_ESCAPE) <> 1 then end 27
	if amiga_test_multikey_queued(&hc5, fb.SC_ESCAPE) <> 0 then end 28
	'' Drain the translated Escape character before continuing the mouse test.
	if inkey <> chr(27) then end 29
	if amiga_test_input(1, 0, 73, 91) <> 0 then end 9
	dim x as integer, y as integer
	if getmouse(x, y) <> 0 then end 10
	if x <> 73 or y <> 91 then end 11
	screen 0
	print "Native graphics pixels and input passed at depth "; depths(mode)
next

'' A1200 ROM modes expose 1280x512 on PAL and 1280x400 on NTSC. A taller
'' canvas must retain that resolution instead of choosing a tiny viewport.
if screenres(1280, 600, 32) <> 0 then end 22
dim nominal_width as long, nominal_height as long
dim mode_id as ulong
if amiga_test_display_mode(nominal_width, nominal_height, mode_id) <> 0 then end 23
'' Native monitor identifiers from graphics/modeid.h.
const MONITOR_ID_MASK = &hffff1000u, PAL_MONITOR_ID = &h00021000u
dim expected_height as long = iif((mode_id and MONITOR_ID_MASK) = PAL_MONITOR_ID, 512, 400)
if nominal_width < 1280 or nominal_height < expected_height then end 24
pset (0, 0), rgb(255, 0, 0)
pset (1279, expected_height - 1), rgb(0, 0, 255)
if amiga_test_display_pixel(0, 0) <> &hff0000 then end 25
if amiga_test_display_pixel(1279, expected_height - 1) <> &h0000ff then end 26
screen 0
print "Native oversized canvas uses "; nominal_width; "x"; nominal_height; " display"
end 0

'' end of graphics.bas
