' TEST_MODE : COMPILE_AND_RUN_OK

''
'' Project: FreeBASIC gfxlib2 tests
'' --------------------------------
''
'' File: simd-put-smoke.bas
''
'' Purpose:
''
''     Verify exact gfxlib2 PUT results across vector-sized spans and scalar
''     tails.
''
'' Responsibilities:
''
''     - exercise logical PUT modes at 8, 16, and 32 bits per pixel
''     - exercise transparent PUT at every supported pixel size
''     - check 16-bit and 32-bit constant blend and additive modes
''     - check 32-bit per-pixel alpha mode
''     - check the indexed alpha-mask conversion into a 32-bit destination
''
'' This file intentionally does NOT contain:
''
''     - performance thresholds
''     - renderer presentation checks
''     - direct instruction-set calls
''

#include once "fbgfx.bi"

const image_width = 19
const image_height = 3
const pixel_count = image_width * image_height
const rgb_mask = &h00FFFFFFu
const transparent_32 = &h00FF00FFu
const transparent_16 = &hF81Fu

dim shared source_32(0 to pixel_count - 1) as ulong
dim shared destination_32(0 to pixel_count - 1) as ulong
dim shared source_16(0 to pixel_count - 1) as ushort
dim shared destination_16(0 to pixel_count - 1) as ushort
dim shared source_8(0 to pixel_count - 1) as ubyte
dim shared destination_8(0 to pixel_count - 1) as ubyte
dim shared source_pixels_16 as ubyte ptr
dim shared destination_pixels_16 as ubyte ptr
dim shared source_pitch_16 as integer
dim shared destination_pitch_16 as integer
dim shared source_pixels_8 as ubyte ptr
dim shared destination_pixels_8 as ubyte ptr
dim shared source_pitch_8 as integer
dim shared destination_pitch_8 as integer

function blend_reference(byval source as ulong, _
	byval destination as ulong, byval factor as ulong) as ulong
	dim result as ulong = 0

	for shift as integer = 0 to 24 step 8
		dim source_component as ulong = (source shr shift) and &hFFu
		dim destination_component as ulong = _
			(destination shr shift) and &hFFu
		dim component as ulong = _
			((source_component * factor) + _
			(destination_component * (256u - factor))) shr 8

		result or= component shl shift
	next
	return result
end function

function add_reference(byval source as ulong, _
	byval destination as ulong, byval factor as ulong) as ulong
	if (source and rgb_mask) = transparent_32 then return destination

	dim result as ulong = 0
	for shift as integer = 0 to 24 step 8
		dim source_component as ulong = (source shr shift) and &hFFu
		dim destination_component as ulong = _
			(destination shr shift) and &hFFu
		dim component as ulong = _
			((source_component * factor) shr 8) + destination_component

		if component > 255u then component = 255u
		result or= component shl shift
	next
	return result
end function

function blend_16_reference(byval source as ushort, _
	byval destination as ushort, byval factor as ulong) as ushort
	if source = transparent_16 then return destination

	dim amount as ulong = (factor + 7u) shr 3
	dim source_rb as ulong = source and &hF81Fu
	dim source_g as ulong = source and &h07E0u
	dim destination_rb as ulong = destination and &hF81Fu
	dim destination_g as ulong = destination and &h07E0u

	source_rb = ((source_rb - destination_rb) * amount) shr 5
	source_g = ((source_g - destination_g) * amount) shr 5
	return ((destination_rb + source_rb) and &hF81Fu) or _
		((destination_g + source_g) and &h07E0u)
end function

function add_16_reference(byval source as ushort, _
	byval destination as ushort, byval factor as ulong) as ushort
	if source = transparent_16 then return destination

	dim amount as ulong = (factor + 7u) shr 3
	dim source_channels as ulong = _
		((culng(source) shl 16) or source) and &h07C0F81Fu
	dim destination_channels as ulong = _
		((culng(destination) shl 16) or destination) and &h07C0F81Fu

	source_channels = _
		((source_channels * amount) shr 5) and &h07C0F81Fu
	source_channels += destination_channels
	dim overflow as ulong = source_channels and &h08010020u
	overflow -= overflow shr 5
	source_channels or= overflow
	source_channels and= &h07C0F81Fu
	source_channels or= source_channels shr 16
	return source_channels and &hFFFFu
end function

sub reset_32(byval image as any ptr)
	for y as integer = 0 to image_height - 1
		for x as integer = 0 to image_width - 1
			pset image, (x, y), destination_32(y * image_width + x)
		next
	next
end sub

sub reset_16(byval image as any ptr)
	for y as integer = 0 to image_height - 1
		for x as integer = 0 to image_width - 1
			dim row as ushort ptr = _
				cptr(ushort ptr, destination_pixels_16 + _
				y * destination_pitch_16)
			row[x] = destination_16(y * image_width + x)
		next
	next
end sub

sub reset_8(byval image as any ptr)
	for y as integer = 0 to image_height - 1
		for x as integer = 0 to image_width - 1
			destination_pixels_8[y * destination_pitch_8 + x] = _
				destination_8(y * image_width + x)
		next
	next
end sub

sub require_32(byval image as any ptr, byval mode_name as string, _
	byval operation as integer, byval factor as ulong = 0)
	for y as integer = 0 to image_height - 1
		for x as integer = 0 to image_width - 1
			dim index as integer = y * image_width + x
			dim source as ulong = source_32(index)
			dim destination as ulong = destination_32(index)
			dim expected as ulong

			select case operation
			case 1
				expected = source and destination
			case 2
				expected = source or destination
			case 3
				expected = source xor destination
			case 4
				expected = not source
			case 5
				if (source and rgb_mask) = transparent_32 then
					expected = destination
				else
					expected = source and rgb_mask
				end if
			case 6
				expected = blend_reference(source, destination, _
					(source shr 24) + 1u)
			case 7
				if (source and rgb_mask) = transparent_32 then
					expected = destination
				else
					expected = blend_reference(source, destination, _
					factor + 1u)
				end if
			case else
				expected = add_reference(source, destination, factor)
			end select

			dim observed as ulong = culng(point(x, y, image))
			if observed <> expected then
				screen 0
				print mode_name; " mismatch at "; x; ","; y; _
					": expected &h"; hex(expected, 8); _
					", observed &h"; hex(observed, 8)
				end 20
			end if
		next
	next
end sub

sub require_16(byval image as any ptr, byval mode_name as string, _
	byval operation as integer, byval factor as ulong = 0)
	for y as integer = 0 to image_height - 1
		for x as integer = 0 to image_width - 1
			dim index as integer = y * image_width + x
			dim source as ushort = source_16(index)
			dim destination as ushort = destination_16(index)
			dim expected as ushort

			select case operation
			case 1
				expected = source and destination
			case 2
				expected = source or destination
			case 3
				expected = source xor destination
			case 4
				expected = not source
			case 5
				if source = transparent_16 then
					expected = destination
				else
					expected = source
				end if
			case 6
				expected = blend_16_reference(source, destination, factor)
			case else
				expected = add_16_reference(source, destination, factor)
			end select

			dim row as ushort ptr = _
				cptr(ushort ptr, destination_pixels_16 + _
				y * destination_pitch_16)
			dim observed as ushort = row[x]
			if observed <> expected then
				screen 0
				print mode_name; " mismatch at "; x; ","; y
				end 21
			end if
		next
	next
end sub

sub require_8(byval image as any ptr, byval mode_name as string, _
	byval operation as integer)
	for y as integer = 0 to image_height - 1
		for x as integer = 0 to image_width - 1
			dim index as integer = y * image_width + x
			dim source as ubyte = source_8(index)
			dim destination as ubyte = destination_8(index)
			dim expected as ubyte

			select case operation
			case 1
				expected = source and destination
			case 2
				expected = source or destination
			case 3
				expected = source xor destination
			case 4
				expected = not source
			case else
				if source = 0u then
					expected = destination
				else
					expected = source
				end if
			end select

			dim observed as ubyte = _
				destination_pixels_8[y * destination_pitch_8 + x]
			if observed <> expected then
				screen 0
				print mode_name; " mismatch at "; x; ","; y
				end 22
			end if
		next
	next
end sub

if screenres(64, 48, 32, 1, fb.GFX_NULL) <> 0 then end 1

dim source_image_32 as any ptr = imagecreate(image_width, image_height, 0, 32)
dim destination_image_32 as any ptr = _
	imagecreate(image_width, image_height, 0, 32)
dim source_image_16 as any ptr = imagecreate(image_width, image_height, 0, 16)
dim destination_image_16 as any ptr = _
	imagecreate(image_width, image_height, 0, 16)
dim source_image_8 as any ptr = imagecreate(image_width, image_height, 0, 8)
dim destination_image_8 as any ptr = _
	imagecreate(image_width, image_height, 0, 8)

if (source_image_32 = 0) or (destination_image_32 = 0) or _
   (source_image_16 = 0) or (destination_image_16 = 0) or _
   (source_image_8 = 0) or (destination_image_8 = 0) then end 2

dim as integer info_width, info_height, info_bpp, info_size
if imageinfo(source_image_16, info_width, info_height, info_bpp, _
	source_pitch_16, source_pixels_16, info_size) <> 0 then end 3
if imageinfo(destination_image_16, info_width, info_height, info_bpp, _
	destination_pitch_16, destination_pixels_16, info_size) <> 0 then end 4
if imageinfo(source_image_8, info_width, info_height, info_bpp, _
	source_pitch_8, source_pixels_8, info_size) <> 0 then end 5
if imageinfo(destination_image_8, info_width, info_height, info_bpp, _
	destination_pitch_8, destination_pixels_8, info_size) <> 0 then end 6

for y as integer = 0 to image_height - 1
	for x as integer = 0 to image_width - 1
		dim index as integer = y * image_width + x
		dim source as ulong = rgba((x * 37 + y * 11) and 255, _
			(x * 17 + y * 53) and 255, (x * 73 + y * 29) and 255, _
			(x * 41 + y * 67) and 255)
		dim destination as ulong = rgba((x * 13 + y * 31) and 255, _
			(x * 61 + y * 7) and 255, (x * 23 + y * 47) and 255, _
			(x * 19 + y * 89) and 255)

		if ((x + y) mod 7) = 0 then
			source = rgba(255, 0, 255, (x * 41 + y * 67) and 255)
		end if
		source_32(index) = source
		destination_32(index) = destination
		source_16(index) = (x * 7919 + y * 1237 + 17) and &hFFFF
		destination_16(index) = (x * 3571 + y * 1879 + 31) and &hFFFF
		source_8(index) = (x * 29 + y * 71 + 3) and &hFF
		destination_8(index) = (x * 47 + y * 13 + 5) and &hFF
		if ((x + y) mod 7) = 0 then source_16(index) = transparent_16
		if ((x + y) mod 5) = 0 then source_8(index) = 0

		pset source_image_32, (x, y), source_32(index)
		dim source_row_16 as ushort ptr = _
			cptr(ushort ptr, source_pixels_16 + y * source_pitch_16)
		source_row_16[x] = source_16(index)
		source_pixels_8[y * source_pitch_8 + x] = source_8(index)
	next
next

reset_32(destination_image_32)
put destination_image_32, (0, 0), source_image_32, and
require_32(destination_image_32, "32-bit AND", 1)
reset_32(destination_image_32)
put destination_image_32, (0, 0), source_image_32, or
require_32(destination_image_32, "32-bit OR", 2)
reset_32(destination_image_32)
put destination_image_32, (0, 0), source_image_32, xor
require_32(destination_image_32, "32-bit XOR", 3)
reset_32(destination_image_32)
put destination_image_32, (0, 0), source_image_32, preset
require_32(destination_image_32, "32-bit PRESET", 4)
reset_32(destination_image_32)
put destination_image_32, (0, 0), source_image_32, trans
require_32(destination_image_32, "32-bit TRANS", 5)
reset_32(destination_image_32)
put destination_image_32, (0, 0), source_image_32, alpha
require_32(destination_image_32, "32-bit ALPHA", 6)
reset_32(destination_image_32)
put destination_image_32, (0, 0), source_image_32, alpha, 137
require_32(destination_image_32, "32-bit BLEND", 7, 137)
reset_32(destination_image_32)
put destination_image_32, (0, 0), source_image_32, add, 173
require_32(destination_image_32, "32-bit ADD", 8, 173)

reset_16(destination_image_16)
put destination_image_16, (0, 0), source_image_16, and
require_16(destination_image_16, "16-bit AND", 1)
reset_16(destination_image_16)
put destination_image_16, (0, 0), source_image_16, or
require_16(destination_image_16, "16-bit OR", 2)
reset_16(destination_image_16)
put destination_image_16, (0, 0), source_image_16, xor
require_16(destination_image_16, "16-bit XOR", 3)
reset_16(destination_image_16)
put destination_image_16, (0, 0), source_image_16, preset
require_16(destination_image_16, "16-bit PRESET", 4)
reset_16(destination_image_16)
put destination_image_16, (0, 0), source_image_16, trans
require_16(destination_image_16, "16-bit TRANS", 5)
reset_16(destination_image_16)
put destination_image_16, (0, 0), source_image_16, alpha, 137
require_16(destination_image_16, "16-bit BLEND", 6, 137)
reset_16(destination_image_16)
put destination_image_16, (0, 0), source_image_16, add, 173
require_16(destination_image_16, "16-bit ADD", 7, 173)

reset_8(destination_image_8)
put destination_image_8, (0, 0), source_image_8, and
require_8(destination_image_8, "8-bit AND", 1)
reset_8(destination_image_8)
put destination_image_8, (0, 0), source_image_8, or
require_8(destination_image_8, "8-bit OR", 2)
reset_8(destination_image_8)
put destination_image_8, (0, 0), source_image_8, xor
require_8(destination_image_8, "8-bit XOR", 3)
reset_8(destination_image_8)
put destination_image_8, (0, 0), source_image_8, preset
require_8(destination_image_8, "8-bit PRESET", 4)
reset_8(destination_image_8)
put destination_image_8, (0, 0), source_image_8, trans
require_8(destination_image_8, "8-bit TRANS", 5)

reset_32(destination_image_32)
put destination_image_32, (0, 0), source_image_8, alpha
for y as integer = 0 to image_height - 1
	for x as integer = 0 to image_width - 1
		dim index as integer = y * image_width + x
		dim expected as ulong = _
			(destination_32(index) and rgb_mask) or _
			(culng(source_8(index)) shl 24)
		if culng(point(x, y, destination_image_32)) <> expected then end 23
	next
next

imagedestroy source_image_32
imagedestroy destination_image_32
imagedestroy source_image_16
imagedestroy destination_image_16
imagedestroy source_image_8
imagedestroy destination_image_8
screen 0
print "GFX2_SIMD_PUT_PASS"
end 0

'' end of simd-put-smoke.bas
