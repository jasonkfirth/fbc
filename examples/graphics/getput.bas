
#include once "fbgfx.bi"

const SCREEN_MODE = 13
const IMAGE_WIDTH = 320
const IMAGE_HEIGHT = 200
const IMAGE_PIXELS = IMAGE_WIDTH * IMAGE_HEIGHT
'' GET writes the current FB.IMAGE header, not the four-byte QB header.
const IMAGE_HEADER_BYTES = sizeof(FB.IMAGE)
const IMAGE_BUFFER_BYTES = IMAGE_HEADER_BYTES + IMAGE_PIXELS

type fb_image field = 1
	header				as FB.IMAGE
	imagedata(0 to IMAGE_PIXELS - 1) as ubyte
end type

type udtfield
	x					as integer
	y					as integer
	array(0 to IMAGE_BUFFER_BYTES - 1) as ubyte
end type

declare sub redraw(byref title as string)

	dim udt as fb_image
	dim udt_ptr as fb_image ptr
	dim array(0 to ((IMAGE_BUFFER_BYTES + sizeof(integer) - 1) \ _
	               sizeof(integer)) - 1) as integer
	dim array_ptr as integer ptr
	dim udtf as udtfield, pudtf as udtfield ptr
	dim k as string

	'' Mode 13 supplies the 320x200x8 layout used by these GET/PUT buffers.
	'' FB-LINTER: DISABLE-NEXT-LINE FBL734
	screen SCREEN_MODE
	if screenptr = 0 then
		print "Unable to create graphics mode 13."
		end 1
	end if

	udt_ptr = @udt
	array_ptr = @array(0)
	pudtf = @udtf

	dim as integer i = 0

	redraw "array": clear array(0),,IMAGE_BUFFER_BYTES
	get (0,0)-(IMAGE_WIDTH-1,IMAGE_HEIGHT-1), array
	cls: put (0,0), array, pset: sleep
	k = inkey

	redraw "array(i)": clear array(0),,IMAGE_BUFFER_BYTES
	get (0,0)-(IMAGE_WIDTH-1,IMAGE_HEIGHT-1), array(i)
	cls: put (0,0), array(i), pset: sleep
	k = inkey

	redraw "@array(i)": clear array(0),,IMAGE_BUFFER_BYTES
	get (0,0)-(IMAGE_WIDTH-1,IMAGE_HEIGHT-1), @array(i)
	cls: put (0,0), @array(i), pset: sleep
	k = inkey

	redraw "array_ptr": clear array(0),,IMAGE_BUFFER_BYTES
	get (0,0)-(IMAGE_WIDTH-1,IMAGE_HEIGHT-1), array_ptr
	cls: put (0,0), array_ptr, pset: sleep
	k = inkey

	redraw "@udt": clear udt,, IMAGE_BUFFER_BYTES
	get (0,0)-(IMAGE_WIDTH-1,IMAGE_HEIGHT-1), @udt
	cls: put (0,0), @udt, pset: sleep
	k = inkey

	'redraw "udt_ptr": clear udt,,IMAGE_BUFFER_BYTES
	'get (0,0)-(IMAGE_WIDTH-1,IMAGE_HEIGHT-1), udt_ptr
	'cls: put (0,0), udt_ptr, pset: sleep
	'k = inkey

	redraw "@array_ptr[i]": clear array(0),,IMAGE_BUFFER_BYTES
	get (0,0)-(IMAGE_WIDTH-1,IMAGE_HEIGHT-1), @array_ptr[i]
	cls: put (0,0), @array_ptr[i], pset: sleep
	k = inkey

	'redraw "@udt_ptr[i]": clear udt,,IMAGE_BUFFER_BYTES
	'get (0,0)-(IMAGE_WIDTH-1,IMAGE_HEIGHT-1), @udt_ptr[i]
	'cls: put (0,0), @udt_ptr[i], pset: sleep
	'k = inkey

	redraw "udt.array": clear udtf.array(0),,IMAGE_BUFFER_BYTES
	get (0,0)-(IMAGE_WIDTH-1,IMAGE_HEIGHT-1), udtf.array
	cls: put (0,0), udtf.array, pset: sleep
	k = inkey

	redraw "udt.array(i)": clear udtf.array(0),,IMAGE_BUFFER_BYTES
	get (0,0)-(IMAGE_WIDTH-1,IMAGE_HEIGHT-1), udtf.array(i)
	cls: put (0,0), udtf.array(i), pset: sleep
	k = inkey

	redraw "@udt.array(i)": clear udtf.array(0),,IMAGE_BUFFER_BYTES
	get (0,0)-(IMAGE_WIDTH-1,IMAGE_HEIGHT-1), @udtf.array(i)
	cls: put (0,0), @udtf.array(i), pset: sleep
	k = inkey

	redraw "udt->array(i)": clear udtf.array(0),,IMAGE_BUFFER_BYTES
	get (0,0)-(IMAGE_WIDTH-1,IMAGE_HEIGHT-1), pudtf->array(i)
	cls: put (0,0), pudtf->array(i), pset: sleep
	k = inkey


sub redraw(byref title as string)
	static as integer c = 1
	cls
	line (0,0)-(IMAGE_WIDTH-1,IMAGE_HEIGHT-1), c
	line (0,IMAGE_HEIGHT-1)-(IMAGE_WIDTH-1,0), c+1
	print title
	c += 1
end sub
