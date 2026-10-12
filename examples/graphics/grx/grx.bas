'' FreeBASIC GRX example
'' File: grx.bas
'' Purpose: Display a 256-color grid using the GRX 2D graphics library.
'' Responsibilities: Mode errors, cell bounds and restoring text mode.
'' This example contains no renderer or platform driver implementation.
'' --------

#include once "grx/grx20.bi"
#include once "grx/grxkeys.bi"

if GrSetMode(GR_width_height_graphics, 320, 240) = 0 then
	print "Could not set the GRX graphics mode."
	end 1
end if

dim as integer x = GrSizeX()
dim as integer y = GrSizeY()
dim as integer ww = (x-10)\32
dim as integer wh = (y-10)\8
if ww <= 0 orElse wh <= 0 then
	GrSetMode(GR_default_text)
	end 1
end if

GrSetRGBcolorMode()
for i as integer = 0 to 7
	for j as integer = 0 to 31
		GrFilledBox(5+j*ww, 5+i*wh, 5+j*ww+ww-1, 5+i*wh+wh-1, i*32+j)
	next
next

dim as integer pixel_error = 0
if environ("FB_EXAMPLE_AUTOPLAY") = "1" then
	'' Give the platform window time to display the completed grid.
	GrSleep(200)
	for i as integer = 0 to 7
		for j as integer = 0 to 31
			if GrPixel(5+j*ww+ww\2, 5+i*wh+wh\2) <> culng(i*32+j) then pixel_error = 1
		next
	next
else
	GrKeyRead()
end if

if GrSetMode(GR_default_text) = 0 then end 1
if pixel_error then end 1
print "GRX color grid completed."

'' end of grx.bas
