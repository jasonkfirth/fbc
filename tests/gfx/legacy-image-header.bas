'' FreeBASIC graphics regressions
'' File: legacy-image-header.bas
'' Purpose: Check the public legacy image header against the C graphics ABI.
'' Responsibilities: Verify metadata, legacy PUT pixels and custom fonts.
'' This file intentionally does NOT contain serialized file-format decoding.

#include "fbcunit.bi"
#include once "fbgfx.bi"

SUITE( fbc_tests.gfx.legacy_image_header )

	TEST( metadata )
		dim pixel_sizes(0 to 2) as integer => {1, 2, 4}
		dim widths(0 to 2) as integer => {1, 20, 8191}
		dim heights(0 to 2) as integer => {1, 17, 65535}
		for pixel_size as integer = 0 to ubound(pixel_sizes)
			for x as integer = 0 to ubound(widths)
				for y as integer = 0 to ubound(heights)
					dim header as fb.IMAGE
					header.old.bpp = pixel_sizes(pixel_size)
					header.old.width = widths(x)
					header.old.height = heights(y)
					dim as integer image_width, image_height, pixel_bytes, image_pitch, image_size
					dim pixels as any ptr
					CU_ASSERT_EQUAL( imageinfo(@header, image_width, image_height, _
						pixel_bytes, image_pitch, pixels, image_size), 0 )
					CU_ASSERT_EQUAL( image_width, widths(x) )
					CU_ASSERT_EQUAL( image_height, heights(y) )
					CU_ASSERT_EQUAL( pixel_bytes, pixel_sizes(pixel_size) )
					CU_ASSERT_EQUAL( image_pitch, widths(x) * pixel_sizes(pixel_size) )
					CU_ASSERT_EQUAL( image_size, 4 + widths(x) * heights(y) * pixel_sizes(pixel_size) )
					CU_ASSERT( pixels = cast(ubyte ptr, @header) + 4 )
				next
			next
		next
	END_TEST

	TEST( pixels )
		type LegacyImage field = 1
			header as fb._OLD_HEADER
			pixels(0 to 3) as ubyte
		end type
		dim image as LegacyImage
		image.header.bpp = 1
		image.header.width = 2
		image.header.height = 2
		for index as integer = 0 to 3
			image.pixels(index) = index + 1
		next
		dim as integer image_width, image_height, pixel_bytes
		CU_ASSERT_EQUAL( imageinfo(@image, image_width, image_height, pixel_bytes), 0 )
		'' A broken header must fail without letting PUT read outside this buffer.
		CU_ASSERT( image_width = 2 and image_height = 2 and pixel_bytes = 1 )
		if image_width <> 2 or image_height <> 2 or pixel_bytes <> 1 then exit sub
		CU_ASSERT_EQUAL( screenres(2, 2, 8, , fb.GFX_NULL), 0 )
		put (0, 0), @image, pset
		for y as integer = 0 to 1
			for x as integer = 0 to 1
				CU_ASSERT_EQUAL( point(x, y), y * 2 + x + 1 )
			next
		next
		screen 0
	END_TEST

	TEST( font )
		type LegacyFont field = 1
			header as fb._OLD_HEADER
			pixels(0 to 47) as ubyte
		end type
		dim image as LegacyFont
		image.header.bpp = 1
		image.header.width = 16
		image.header.height = 3
		'' One two-pixel glyph, with a two-row diagonal pattern below its header.
		image.pixels(0) = 0
		image.pixels(1) = 65
		image.pixels(2) = 65
		image.pixels(3) = 2
		image.pixels(16) = 1
		image.pixels(33) = 1
		dim as long measured_width, measured_height
		dim result as long = fb.DrawStringSize("A", measured_width, measured_height, @image)
		CU_ASSERT_EQUAL( result, 0 )
		CU_ASSERT_EQUAL( measured_width, 2 )
		CU_ASSERT_EQUAL( measured_height, 2 )
		if result <> 0 or measured_width <> 2 or measured_height <> 2 then exit sub
		CU_ASSERT_EQUAL( screenres(4, 4, 8, , fb.GFX_NULL), 0 )
		for implicit_depth as integer = 0 to 1
			'' QB buffers may leave bpp zero and inherit the destination depth.
			image.header.bpp = iif(implicit_depth, 0, 1)
			line (0, 0)-(3, 3), 6, bf
			draw string (1, 1), "A", , @image, pset
			for y as integer = 0 to 3
				for x as integer = 0 to 3
					dim expected as integer = 6
					if x >= 1 and x <= 2 and y >= 1 and y <= 2 then
						expected = iif(x = y, 1, 0)
					end if
					CU_ASSERT_EQUAL( point(x, y), expected )
				next
			next
		next
		screen 0
	END_TEST

END_SUITE

'' end of legacy-image-header.bas
