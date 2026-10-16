'' Project: FreeBASIC classic AmigaOS image qualification
'' File: image-colours.bas
'' Purpose: Verify packed BMP channels through the public graphics API.
'' Responsibilities: Write endian-independent fixtures and check image colours.
'' This file intentionally does NOT contain corpus adaptations or a BMP decoder.

#include once "fbgfx.bi"

private sub WriteLe16(byval file_number as integer, byval value as ulong)
	for index as integer = 0 to 1
		dim octet as ubyte = (value shr (index * 8)) and 255
		put #file_number, , octet
	next
end sub

private sub WriteLe32(byval file_number as integer, byval value as ulong)
	for index as integer = 0 to 3
		dim octet as ubyte = (value shr (index * 8)) and 255
		put #file_number, , octet
	next
end sub

private sub WriteBitmapHeader(byval file_number as integer, byval pixel_width as ulong, _
	byval red_mask as ulong, byval green_mask as ulong, byval blue_mask as ulong, _
	byval alpha_mask as ulong)
	'' BITMAPFILEHEADER (14 bytes), BITMAPV4HEADER (108 bytes), then pixels.
	WriteLe16(file_number, &h4d42)
	WriteLe32(file_number, 122 + pixel_width * 4)
	WriteLe32(file_number, 0)
	WriteLe32(file_number, 122)
	WriteLe32(file_number, 108)
	WriteLe32(file_number, pixel_width)
	WriteLe32(file_number, 1)
	WriteLe16(file_number, 1)
	WriteLe16(file_number, 32)
	WriteLe32(file_number, 3) '' BI_BITFIELDS
	WriteLe32(file_number, pixel_width * 4)
	for index as integer = 0 to 3
		WriteLe32(file_number, 0)
	next
	WriteLe32(file_number, red_mask)
	WriteLe32(file_number, green_mask)
	WriteLe32(file_number, blue_mask)
	WriteLe32(file_number, alpha_mask)
	'' The remaining V4 colour-space, endpoints, and gamma fields are zero.
	for index as integer = 0 to 12
		WriteLe32(file_number, 0)
	next
end sub

private function ExpandSample(byval value as ulong, byval source_bits as integer, _
	byval target_bits as integer) as ulong
	'' Construct the reference one bit at a time, independently of the decoder.
	dim result as ulong = 0
	for index as integer = 0 to target_bits - 1
		dim source_index as integer = source_bits - 1 - (index mod source_bits)
		result = (result shl 1) or ((value shr source_index) and 1)
	next
	return result
end function

const filename = "native-image-colours.bmp"
dim file_number as integer = freefile()
if open(filename for binary access write as #file_number) <> 0 then end 1
WriteBitmapHeader(file_number, 4, &h3ff00000, &h000ffc00, &h000003ff, &hc0000000)
for opacity as integer = 0 to 3
	dim packed as ulong = (18ul shl 22) or (52ul shl 12) or (86ul shl 2) or (culng(opacity) shl 30)
	WriteLe32(file_number, packed)
next
close #file_number

if screenres(16, 16, 32, , fb.GFX_NULL) <> 0 then end 2
dim image as any ptr = imagecreate(4, 1, 0, 32)
if image = 0 then end 3
if bload(filename, image) <> 0 then end 4
for opacity as integer = 0 to 3
	dim expected as ulong = rgba(18, 52, 86, opacity * 85)
	dim observed as ulong = point(opacity, 0, image)
	if expected <> observed then
		imagedestroy image
		screen 0
		print "BMP channel mismatch: opacity="; opacity; " expected="; hex(expected, 8); " observed="; hex(observed, 8)
		end 5
	end if
next
put (0, 0), image, alpha
if point(3, 0) <> rgba(18, 52, 86, 255) then end 6
imagedestroy image
screen 0

dim target_depths(0 to 1) as integer => {16, 32}
for mode as integer = 0 to 1
	if screenres(16, 16, target_depths(mode), , fb.GFX_NULL) <> 0 then end 7
	for source_bits as integer = 1 to 8
		dim mask as ulong = (1ul shl source_bits) - 1
		dim pixel_width as integer = mask + 1
		if open(filename for binary access write as #file_number) <> 0 then end 8
		WriteBitmapHeader(file_number, pixel_width, mask shl (source_bits * 2), _
			mask shl source_bits, mask, mask shl (source_bits * 3))
		for value as ulong = 0 to mask
			dim packed as ulong = (((value * 3) and mask) shl (source_bits * 2)) or _
				(((value * 5) and mask) shl source_bits) or ((value * 7) and mask) or _
				(value shl (source_bits * 3))
			WriteLe32(file_number, packed)
		next
		close #file_number
		image = imagecreate(pixel_width, 1, 0, target_depths(mode))
		if image = 0 then end 9
		if bload(filename, image) <> 0 then end 10
		for value as ulong = 0 to mask
			dim red as ulong = (value * 3) and mask
			dim green as ulong = (value * 5) and mask
			dim blue as ulong = (value * 7) and mask
			dim expected as ulong
			if target_depths(mode) = 16 then
				red = ExpandSample(ExpandSample(red, source_bits, 5), 5, 8)
				green = ExpandSample(ExpandSample(green, source_bits, 6), 6, 8)
				blue = ExpandSample(ExpandSample(blue, source_bits, 5), 5, 8)
				expected = rgb(red, green, blue) and &hffffff
			else
				expected = rgba(ExpandSample(red, source_bits, 8), _
					ExpandSample(green, source_bits, 8), ExpandSample(blue, source_bits, 8), _
					ExpandSample(value, source_bits, 8))
			end if
			dim observed as ulong = point(value, 0, image)
			if expected <> observed then
				imagedestroy image
				screen 0
				print "BMP channel mismatch: depth="; target_depths(mode); " bits="; source_bits; _
					" value="; value; " expected="; hex(expected, 8); " observed="; hex(observed, 8)
				end 11
			end if
		next
		imagedestroy image
	next
	screen 0
next
kill filename
print "NATIVE_IMAGE_COLOURS_OK"
end 0

'' end of image-colours.bas
