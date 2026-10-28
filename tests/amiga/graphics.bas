'' FreeBASIC classic AmigaOS graphics qualification
'' ----------------------------------------------
'' File: graphics.bas
'' Purpose: Verify native pixels and input across framebuffer formats.
'' Responsibilities: Draw colour bands, inspect the display, and exercise keys.
'' This file intentionally does NOT contain display or message implementation.

#include once "fbgfx.bi"

extern "C"
	declare function amiga_test_window_layout(byval borderless as long) as long
	declare function amiga_test_display_pixel(byval x as long, byval y as long) as ulong
	declare function amiga_test_input(byval kind as long, byval code as long, _
		byval x as long, byval y as long) as long
	declare function amiga_test_multikey_queued(byval code as long, _
		byval scancode as long) as long
	declare function amiga_test_event_queued(byval code as long, byval peek as long) as long
	declare function amiga_test_pointer_hidden() as long
	declare function amiga_test_present_cache(byval x as long, byval y as long) as long
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

'' True-colour palette fixtures inspect native RGB after palette transitions.
private function FrameRGB(byval colour as ulong, byval depth as integer) as ulong
	dim red as integer = (colour shr 16) and 255
	dim green as integer = (colour shr 8) and 255
	dim blue as integer = colour and 255
	if depth = 16 then
		red = ((red shr 3) * 255) \ 31
		green = ((green shr 2) * 255) \ 63
		blue = ((blue shr 3) * 255) \ 31
	end if
	return rgb(red, green, blue) and &hffffff
end function

private sub RequireDisplayRGB(byval x as integer, byval y as integer, _
	byval expected as ulong, byval stage as integer)
	dim observed as ulong = amiga_test_display_pixel(x, y)
	if observed <> expected then
		screen 0
		print "True-colour palette mismatch: stage="; stage; " x="; x; " y="; y; _
			" expected="; hex(expected, 6); " observed="; hex(observed, 6)
		end 63
	end if
end sub

private sub TestTrueColourPalette(byval depth as integer)
	dim stay as ulong = rgb(93, 17, 203)
	dim levels3(0 to 7) as integer => {0, 36, 73, 109, 146, 182, 219, 255}
	dim levels2(0 to 3) as integer => {0, 85, 170, 255}
	'' Both true-colour formats must preserve every colour when the source
	'' plus stable UI pens fits. The 240 distinct colours also exercise hash
	'' collisions without relying on a particular native pen assignment.
	screenlock
	line (0, 0)-(318, 199), rgb(0, 0, 0), bf
	pset (200, 5), stay
	for index as integer = 0 to 239
		pset (index, 199), rgb((index and 31) * 8, (index shr 5) * 4 + 4, 17)
	next
	screenunlock
	for index as integer = 0 to 239
		RequireDisplayRGB(index, 199, FrameRGB( _
			rgb((index and 31) * 8, (index shr 5) * 4 + 4, 17), depth), 1)
	next
	RequireDisplayRGB(200, 5, FrameRGB(stay, depth), 1)

	'' Replace colours until append-only allocation fills. The new earlier
	'' pixel changes scan order when unused old colours are discarded, so
	'' the unchanged row containing 'stay' must also be uploaded again.
	screenlock
	pset (0, 0), rgb(201, 69, 177)
	for index as integer = 0 to 239
		pset (index, 199), rgb((index and 31) * 8, (index shr 5) * 4 + 4, 145)
	next
	screenunlock
	RequireDisplayRGB(0, 0, FrameRGB(rgb(201, 69, 177), depth), 2)
	RequireDisplayRGB(200, 5, FrameRGB(stay, depth), 2)
	for index as integer = 0 to 239
		RequireDisplayRGB(index, 199, FrameRGB( _
			rgb((index and 31) * 8, (index shr 5) * 4 + 4, 145), depth), 2)
	next

	'' This row has 319 distinct colours even after RGB565 normalization.
	'' Returning to the fixed palette must repaint the untouched row too.
	screenlock
	for index as integer = 0 to 318
		pset (index, 0), rgb((index and 31) * 8, _
			((index shr 5) and 63) * 4, ((index * 7) and 31) * 8)
	next
	screenunlock
	dim converted as ulong = FrameRGB(stay, depth)
	dim red as integer = (converted shr 16) and 255
	dim green as integer = (converted shr 8) and 255
	dim blue as integer = converted and 255
	RequireDisplayRGB(200, 5, rgb(NearestLevel(red, levels3()), _
		NearestLevel(green, levels3()), NearestLevel(blue, levels2())) and &hffffff, 3)

	'' Erase only the rich row. The low-colour frame fits again, and the
	'' unchanged sample must recover its exact RGB without another PSET.
	line (0, 0)-(318, 0), rgb(0, 0, 0)
	RequireDisplayRGB(200, 5, FrameRGB(stay, depth), 4)
end sub


private sub TestDominantPalette(byval depth as integer)
	'' Antialiased UI text can exceed the exact pen budget while almost all
	'' pixels still use a handful of flat colours. Preserve those regions.
	screenlock
	line (0, 0)-(318, 199), rgb(240, 240, 240), bf
	line (20, 20)-(100, 100), rgb(128, 128, 128), bf
	line (120, 20)-(200, 100), rgb(0, 120, 215), bf
	for index as integer = 0 to 319
		pset (index mod 160, index \ 160), rgb((index and 31) * 8, _
			(index shr 5) * 4, 215)
	next
	for index as integer = 0 to 127
		pset (index, 2), rgb(index * 2, index * 2, index * 2)
	next
	screenunlock
	RequireDisplayRGB(318, 199, FrameRGB(rgb(240, 240, 240), depth), 6)
	RequireDisplayRGB(50, 50, FrameRGB(rgb(128, 128, 128), depth), 6)
	RequireDisplayRGB(150, 50, FrameRGB(rgb(0, 120, 215), depth), 6)


	'' Keep rare colour bands within two RGB565 component steps. With a
	'' maximum expanded five-bit step of nine, this fixture permits eighteen
	'' levels rather than allowing distant grey pens to replace coloured edges.
	for index as integer = 0 to 319
		dim expected as ulong = FrameRGB(rgb((index and 31) * 8, _
			(index shr 5) * 4, 215), depth)
		dim observed as ulong = amiga_test_display_pixel(index mod 160, index \ 160)
		for component as integer = 0 to 2
			dim channel_bits as integer = component * 8
			if abs(cint((expected shr channel_bits) and 255) - _
				cint((observed shr channel_bits) and 255)) > 18 then end 64
		next
	next

	'' A new large region changes palette priorities. The untouched grey
	'' block must retain its colour after every pen assignment is rebuilt.
	line (201, 3)-(318, 199), rgb(212, 208, 200), bf
	RequireDisplayRGB(318, 199, FrameRGB(rgb(212, 208, 200), depth), 7)
	RequireDisplayRGB(50, 50, FrameRGB(rgb(128, 128, 128), depth), 7)
	RequireDisplayRGB(150, 50, FrameRGB(rgb(0, 120, 215), depth), 7)
end sub

private sub TestBorderlessPalette(byval depth as integer)
	'' Black backdrop plus 255 distinct colours fills every available pen.
	'' No window UI pens may reduce that capacity in a borderless display.
	screenlock
	line (0, 0)-(318, 199), rgb(0, 0, 0), bf
	for index as integer = 1 to 255
		pset (index, 100), rgb((index and 31) * 8, (index shr 5) * 4 + 4, 17)
	next
	screenunlock
	RequireDisplayRGB(0, 100, 0, 5)
	for index as integer = 1 to 255
		RequireDisplayRGB(index, 100, FrameRGB( _
			rgb((index and 31) * 8, (index shr 5) * 4 + 4, 17), depth), 5)
	next
end sub

dim depths(0 to 2) as integer => {8, 16, 32}
screencontrol fb.SET_DRIVER_NAME, "AMIGA"
for mode as integer = 0 to 2
	if screenres(319, 200, depths(mode), 2) <> 0 then end 1
	if amiga_test_window_layout(0) <> 0 then end 55
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

	'' Updating the native title must not draw an Intuition title bar over
	'' the pixels of a borderless private screen, including unchanged rows.
	windowtitle "Updated graphics qualification title"
	for stripe as integer = 0 to 7
		dim expected as ulong = rgb((stripe and 1) * 255, _
			((stripe shr 1) and 1) * 255, ((stripe shr 2) and 1) * 255) and &hffffff
		if amiga_test_display_pixel(stripe * 40 + 10, 5) <> expected then end 51
	next

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
			expected = rgb(red, green, blue) and &hffffff
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
	'' Identical rows may skip planar transfers, but window refreshes must
	'' repaint damaged pixels. Missing optional cache memory must also work.
	if amiga_test_present_cache(24, 24) <> 0 then end 30
	if amiga_test_display_pixel(24, 24) <> &hff0000 then end 31

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
	'' Arrow keys must retain the standard two-byte INKEY representation.
	'' Native messages keep this independent of emulator joystick mappings.
	dim raw_arrow(0 to 3) as integer => {&h4c, &h4d, &h4e, &h4f}
	dim fb_arrow(0 to 3) as integer => {fb.SC_UP, fb.SC_DOWN, fb.SC_RIGHT, fb.SC_LEFT}
	for arrow as integer = 0 to 3
		if amiga_test_input(0, raw_arrow(arrow), 0, 0) <> 0 then end 32
		if not multikey(fb_arrow(arrow)) then end 33
		if inkey <> chr(255) + chr(fb_arrow(arrow)) then end 34
		if amiga_test_input(0, raw_arrow(arrow) or &h80, 0, 0) <> 0 then end 35
		if multikey(fb_arrow(arrow)) then end 36
	next
	'' SCREENEVENT-only loops must consume native messages without an INKEY
	'' or MULTIKEY call first. Peeking must leave the same event available.
	if amiga_test_event_queued(&h20, 0) <> 0 then end 48
	if inkey <> "a" then end 49
	if amiga_test_event_queued(&ha0, 1) <> 0 then end 50
	if multikey(fb.SC_A) then end 51
	if amiga_test_input(1, 0, 73, 91) <> 0 then end 9
	dim x as integer, y as integer
	if getmouse(x, y) <> 0 then end 10
	if x <> 73 or y <> 91 then end 11
	if depths(mode) > 8 then
		TestTrueColourPalette(depths(mode))
		TestDominantPalette(depths(mode))
	end if
	screen 0
	if depths(mode) > 8 then print "Native exact palette and fallback transitions passed at depth "; depths(mode)
	print "Native graphics pixels and input passed at depth "; depths(mode)
next

'' A1200 ROM modes expose 1280x512 on PAL and 1280x400 on NTSC. A taller
'' canvas must retain that resolution instead of choosing a tiny viewport.
if screenres(1280, 600, 32) <> 0 then end 22
if amiga_test_window_layout(0) <> 0 then end 56
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
for mode as integer = 0 to 2
	for form as integer = 0 to 1
		dim flags as integer = iif(form = 0, fb.GFX_FULLSCREEN, fb.GFX_NO_FRAME)
		if screenres(319, 200, depths(mode), 2, flags) <> 0 then end 57
		if amiga_test_window_layout(1) <> 0 then end 58
		pset (0, 0), iif(depths(mode) = 8, 15, rgb(255, 255, 255))
		if amiga_test_display_pixel(0, 0) <> &hffffff then end 59
		if depths(mode) > 8 then TestBorderlessPalette(depths(mode))
		screen 0
	next
next
print "Native windowed/fullscreen/borderless client layouts passed"

end 0

'' end of graphics.bas
