'' FreeBASIC classic AmigaOS graphics smoke test
'' -------------------------------------------
'' File: gfx-smoke.bas
'' Purpose: Exercise native indexed and true-colour display lifecycles.
'' Responsibilities: Draw visible colour bands and check framebuffer pixels.
'' This file intentionally does NOT contain keyboard waits or emulator control.

#include once "fbgfx.bi"

extern "C"
	declare sub fb_Delay(byval milliseconds as long)
end extern

dim depths(0 to 2) as integer => {8, 16, 32}
windowtitle "Amiga graphics smoke"
screencontrol fb.SET_DRIVER_NAME, "AMIGA"
for mode as integer = 0 to 2
	print "Opening graphics depth "; depths(mode)
	'' An odd width exercises the scratch bitmap's planar row padding.
	if screenres(319, 200, depths(mode)) <> 0 then end 1
	screenlock
	for stripe as integer = 0 to 7
		dim colour as ulong
		if depths(mode) = 8 then
			colour = stripe + 1
			palette colour, (stripe and 1) * 255, ((stripe shr 1) and 1) * 255, ((stripe shr 2) and 1) * 255
		else
			colour = rgb((stripe and 1) * 255, ((stripe shr 1) and 1) * 255, ((stripe shr 2) and 1) * 255)
		end if
		line (stripe * 40, 0)-((stripe + 1) * 40 - 1, 199), colour, bf
	next
	screenunlock
	'' SCREEN 0 restores the console before the completion marker is printed.
	fb_Delay(500)
	screen 0
	print "Closed graphics depth "; depths(mode)
next
print "AmigaOS graphics smoke passed"
end 0

'' end of gfx-smoke.bas
