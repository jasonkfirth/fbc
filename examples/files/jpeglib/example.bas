''
'' Project: FreeBASIC examples
'' --------------------------
''
'' File: example.bas
''
'' Purpose:
''
''     Load grayscale and RGB JPEG images through the libjpeg API.
''
'' Responsibilities:
''
''     - validate graphics setup and decoder output
''     - convert decoded rows into FreeBASIC image storage
''     - release file, decoder, row-buffer, and image resources on failure
''
'' Ownership:
''
''     imageread_jpg transfers its completed image to the caller.  Its file,
''     decoder, row storage, and partial image remain local until that point.
''
'' This file intentionally does NOT contain:
''
''     - JPEG writing support
''     - CMYK or YCCK color conversion
''
'' Select __JPEGLIB_VER__ to match the SDK's JPEG_LIB_VERSION. The Windows
'' libjpeg-turbo ABI 8 builds use -d __JPEGLIB_VER__=8.
''
'' This example works with both fb and fblite dialects
'#lang "fblite"

#include once "fbgfx.bi"
#include once "jpeglib.bi"
#include once "crt.bi"
#include once "crt/setjmp.bi"

const SCR_W = 640
const SCR_H = 480
const SCR_BPP = 32

declare function imageread_jpg _
	( _
		byref filename as string, _
		byval bpp as integer _
	) as any ptr

#ifndef IMAGE_EXAMPLE_NO_MAIN
	if( screenres( SCR_W, SCR_H, SCR_BPP ) <> 0 ) then
		print "could not set graphics mode"
		end 1
	end if

	#if SCR_BPP <= 8
		'' With 8bpp or less screen, a palette is used, instead of RGB colors.
		'' We need to have a palette that matches the colors used in the images
		'' that we want to load in this case.

		'' Set global palette to 0..255 grayscale, matching the
		'' grayscale fblogo.jpg:
		for i as integer = 0 to 255
			palette i, i, i, i
		next

		dim as any ptr img1
		dim filename1 as string = command(1)
		if filename1 = "" then filename1 = exepath( ) & "/../../fblogo.jpg"
		img1 = imageread_jpg( filename1, SCR_BPP )
		if( img1 = NULL ) then
			end 1
		end if

		put (0, 0), img1, pset

		sleep

		imagedestroy( img1 )
	#else
		dim as any ptr img1, img2

		dim filename1 as string = command(1)
		if filename1 = "" then filename1 = exepath( ) & "/../../fblogo.jpg"
		img1 = imageread_jpg( filename1, SCR_BPP )
		dim filename2 as string = command(2)
		if filename2 = "" then filename2 = exepath( ) & "/color.jpg"
		img2 = imageread_jpg( filename2, SCR_BPP )
		if( (img1 = NULL) or (img2 = NULL) ) then
			if( img2 <> NULL ) then imagedestroy( img2 )
			if( img1 <> NULL ) then imagedestroy( img1 )
			end 1
		end if

		put (0, 0), img1, pset
		put (320, 0), img2, pset

		sleep

		imagedestroy( img2 )
		imagedestroy( img1 )
	#endif
#endif

'' The error manager returns through setjmp instead of ending the process.
'' Keep every acquired resource in heap state: locals modified after setjmp
'' are not reliable after longjmp, and managed FB strings must not be skipped.
type jpeg_example_state
	info as jpeg_decompress_struct
	errors as jpeg_error_mgr
	jump as jmp_buf ptr
	file_handle as FILE ptr
	row(0 to 0) as JSAMPROW
	image as any ptr
	decoder_created as integer
end type

private sub imageread_jpg_cleanup(byval state as jpeg_example_state ptr)
	if state = NULL then exit sub
	if state->row(0) <> NULL then deallocate(state->row(0))
	if state->decoder_created then jpeg_destroy_decompress(@state->info)
	if state->file_handle <> NULL then fclose(state->file_handle)
	if state->image <> NULL then imagedestroy(state->image)
	if state->jump <> NULL then deallocate(state->jump)
	deallocate(state)
end sub

private sub imageread_jpg_error cdecl(byval info as j_common_ptr)
	dim state as jpeg_example_state ptr = cast(jpeg_example_state ptr, info->client_data)
	'' libjpeg requires error_exit to leave the decoder, not return into it.
	info->err->output_message(info)
	if state = NULL orelse state->jump = NULL then end 1
	longjmp(state->jump, 1)
end sub

function imageread_jpg(byref filename as string, byval bpp as integer) as any ptr
	if filename = "" then return NULL
	select case bpp
	case 8, 15, 16, 24, 32
	case else
		return NULL
	end select

	dim state as jpeg_example_state ptr = callocate(sizeof(jpeg_example_state))
	if state = NULL then return NULL
	'' A separately allocated buffer keeps the Win64 CRT's 16-byte jump-buffer
	'' alignment even when the FB record containing the error manager uses 8.
	state->jump = callocate(sizeof(jmp_buf))
	if state->jump = NULL then
		imageread_jpg_cleanup(state)
		return NULL
	end if
#ifdef __FB_64BIT__
#ifdef __FB_WIN32__
	if (cast(uinteger, state->jump) and 15) <> 0 then
		imageread_jpg_cleanup(state)
		return NULL
	end if
#endif
#endif
	state->file_handle = fopen(filename, "rb")
	if state->file_handle = NULL then
		print "could not open image file "; filename
		imageread_jpg_cleanup(state)
		return NULL
	end if

	state->info.err = jpeg_std_error(@state->errors)
	state->errors.error_exit = @imageread_jpg_error
	state->info.client_data = state
	if setjmp(state->jump) <> 0 then
		imageread_jpg_cleanup(state)
		return NULL
	end if
	'' The zeroed memory-manager pointer also permits cleanup if creation fails.
	state->decoder_created = -1
	jpeg_create_decompress(@state->info)
	jpeg_stdio_src(@state->info, state->file_handle)
	if jpeg_read_header(@state->info, 1) <> JPEG_HEADER_OK then
		imageread_jpg_cleanup(state)
		return NULL
	end if
	'' This display example permits at most 16 million pixels (64 MiB at 32 bpp).
	'' Check before starting decompression, which can acquire large buffers.
	const MAX_IMAGE_PIXELS = 16 * 1024 * 1024
	if state->info.image_width = 0 or state->info.image_height = 0 or _
	   culngint(state->info.image_width) * state->info.image_height > MAX_IMAGE_PIXELS then
		imageread_jpg_cleanup(state)
		return NULL
	end if
	if jpeg_start_decompress(@state->info) = 0 then
		imageread_jpg_cleanup(state)
		return NULL
	end if
	select case state->info.out_color_space
	case JCS_GRAYSCALE
		if state->info.output_components <> 1 then
			imageread_jpg_cleanup(state)
			return NULL
		end if
	case JCS_RGB
		if state->info.output_components <> 3 then
			imageread_jpg_cleanup(state)
			return NULL
		end if
	case else
		print "JPEG color space not supported by this example"
		imageread_jpg_cleanup(state)
		return NULL
	end select
	dim rowbytes as integer = state->info.output_width * state->info.output_components
	if rowbytes <= 0 then
		imageread_jpg_cleanup(state)
		return NULL
	end if
	state->row(0) = callocate(rowbytes)
	state->image = imagecreate(state->info.output_width, state->info.output_height, 0, bpp)
	if state->row(0) = NULL or state->image = NULL then
		imageread_jpg_cleanup(state)
		return NULL
	end if

	'' Omitting width/height selects IMAGEINFO's 32-bit Long overload.
	dim pitch as long
	dim image_data as any ptr
	if imageinfo(state->image, , , , pitch, image_data) <> 0 or image_data = NULL then
		imageread_jpg_cleanup(state)
		return NULL
	end if
	while state->info.output_scanline < state->info.output_height
		dim row_index as integer = state->info.output_scanline
		if jpeg_read_scanlines(@state->info, @state->row(0), 1) <> 1 then
			imageread_jpg_cleanup(state)
			return NULL
		end if
		'' IMAGEINFO supplies the real pixel address and padded row stride in
		'' both dialects. Never add one byte to the image-header pointer.
		dim dst as ubyte ptr = cast(ubyte ptr, image_data) + row_index * pitch
		dim src as ubyte ptr = state->row(0)
		if state->info.out_color_space = JCS_GRAYSCALE then
			dim i as integer
			select case bpp
			case 24, 32
				for i = 0 to rowbytes-1
					'' The checked row allocation contains every sample. FB-LINTER: DISABLE-NEXT-LINE FBL525
					cast(ulong ptr, dst)[i] = rgb(src[i], src[i], src[i])
				next
			case 15, 16
				for i = 0 to rowbytes-1
					'' The checked row allocation contains every sample. FB-LINTER: DISABLE-NEXT-LINE FBL525
					pset state->image, (i, row_index), rgb(src[i], src[i], src[i])
				next
			case else
				'' Indexed images require a matching global grayscale palette.
				memcpy(dst, src, rowbytes)
			end select
		else
			imageconvertrow(src, 24, dst, bpp, state->info.output_width)
		end if
	wend
	if jpeg_finish_decompress(@state->info) = 0 then
		imageread_jpg_cleanup(state)
		return NULL
	end if
	dim completed_image as any ptr = state->image
	state->image = NULL
	imageread_jpg_cleanup(state)
	return completed_image
end function

'' end of example.bas
