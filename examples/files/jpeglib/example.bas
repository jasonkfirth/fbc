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
'' This example works with both fb and fblite dialects
'#lang "fblite"

#include once "fbgfx.bi"
#include once "jpeglib.bi"
#include once "crt.bi"

const SCR_W = 640
const SCR_H = 480
const SCR_BPP = 32

declare function imageread_jpg _
	( _
		byref filename as string, _
		byval bpp as integer _
	) as any ptr


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
		img1 = imageread_jpg( exepath( ) & "/../../fblogo.jpg", SCR_BPP )
		if( img1 = NULL ) then
			sleep : end 1
		end if

		put (0, 0), img1, pset

		sleep

		imagedestroy( img1 )
	#else
		dim as any ptr img1, img2

		img1 = imageread_jpg( exepath( ) & "/../../fblogo.jpg", SCR_BPP )
		img2 = imageread_jpg( exepath( ) & "/color.jpg", SCR_BPP )
		if( (img1 = NULL) or (img2 = NULL) ) then
			if( img2 <> NULL ) then imagedestroy( img2 )
			if( img1 <> NULL ) then imagedestroy( img1 )
			sleep : end 1
		end if

		put (0, 0), img1, pset
		put (320, 0), img2, pset

		sleep

		imagedestroy( img2 )
		imagedestroy( img1 )
	#endif

private sub imageread_jpg_cleanup _
	( _
		byref fp as FILE ptr, _
		byref jinfo as jpeg_decompress_struct, _
		byval decoder_created as integer, _
		byref rows as JSAMPARRAY, _
		byref img as byte ptr _
	)

	if( rows <> NULL ) then
		if( rows[0] <> NULL ) then
			deallocate( rows[0] )
			rows[0] = NULL
		end if
		deallocate( rows )
		rows = NULL
	end if
	if( decoder_created <> 0 ) then
		jpeg_destroy_decompress( @jinfo )
	end if
	if( fp <> NULL ) then
		fclose( fp )
		fp = NULL
	end if
	if( img <> NULL ) then
		imagedestroy( img )
		img = NULL
	end if

end sub

function imageread_jpg _
	( _
		byref filename as string, _
		byval bpp as integer _
	) as any ptr

	dim as FILE ptr fp
	dim jinfo as jpeg_decompress_struct
	dim jerr as jpeg_error_mgr
	dim as integer decoder_created
	dim as integer rowbytes
	dim rows as JSAMPARRAY
	dim src as ubyte ptr
	dim img as byte ptr

	function = NULL

	fp = fopen( filename, "rb" )
	if( fp = NULL ) then
		print "could not open image file " & filename
		return NULL
	end if

	jinfo.err = jpeg_std_error( @jerr )
	jpeg_create_decompress( @jinfo )
	decoder_created = -1

	jpeg_stdio_src( @jinfo, fp )
	jpeg_read_header( @jinfo, 1 )
	jpeg_start_decompress( @jinfo )

	select case( jinfo.out_color_space )
	case JCS_GRAYSCALE
		if( (jinfo.output_components <> 1) or (jinfo.out_color_components <> 1) ) then
			'' grayscale, but not 1 byte per pixel (will this ever happen?)
			imageread_jpg_cleanup( fp, jinfo, decoder_created, rows, img )
			return NULL
		end if
	case JCS_RGB
		if( (jinfo.output_components <> 3) or (jinfo.out_color_components <> 3) ) then
			'' RGB, but not 3 bytes per pixel (will this ever happen?)
			imageread_jpg_cleanup( fp, jinfo, decoder_created, rows, img )
			return NULL
		end if
	case JCS_YCbCr
		print "jpeg image uses YCbCr color space, not implemented"
		imageread_jpg_cleanup( fp, jinfo, decoder_created, rows, img )
		return NULL
	case JCS_CMYK
		print "jpeg image uses CMYK color space, not implemented"
		imageread_jpg_cleanup( fp, jinfo, decoder_created, rows, img )
		return NULL
	case JCS_YCCK
		print "jpeg image uses YCCK color space, not implemented"
		imageread_jpg_cleanup( fp, jinfo, decoder_created, rows, img )
		return NULL
	case else
		print "jpeg image uses unknown color space"
		imageread_jpg_cleanup( fp, jinfo, decoder_created, rows, img )
		return NULL
	end select

	rowbytes = jinfo.output_width * jinfo.output_components
	if( rowbytes <= 0 ) then
		print "invalid JPEG row size"
		imageread_jpg_cleanup( fp, jinfo, decoder_created, rows, img )
		return NULL
	end if

	'' Allocate an array of rows (but with only 1 row, since we're going to
	'' read only one at a time)
	rows = callocate( sizeof( ubyte ptr ) )
	if( rows = NULL ) then
		print "could not allocate JPEG row table"
		imageread_jpg_cleanup( fp, jinfo, decoder_created, rows, img )
		return NULL
	end if

	rows[0] = callocate( rowbytes )
	if( rows[0] = NULL ) then
		print "could not allocate JPEG row buffer"
		imageread_jpg_cleanup( fp, jinfo, decoder_created, rows, img )
		return NULL
	end if

	src = rows[0]

#if __FB_LANG__ = "fb"
	img = imagecreate( jinfo.output_width, jinfo.output_height )
	if( img = NULL ) then
		print "could not allocate output image"
		imageread_jpg_cleanup( fp, jinfo, decoder_created, rows, img )
		return NULL
	end if

	dim pitch as integer = cptr( FB.IMAGE ptr, img )->pitch
	dim as ubyte ptr dst = cast( ubyte ptr, img + 1 )
#else
	img = imagecreate( jinfo.output_width, jinfo.output_height )
	if( img = NULL ) then
		print "could not allocate output image"
		imageread_jpg_cleanup( fp, jinfo, decoder_created, rows, img )
		return NULL
	end if

	'' IMAGEINFO's legacy fblite overload exposes a 32-bit Long pitch.
	dim pitch as long
	imageinfo img, , , , pitch
	dim as ubyte ptr dst = cast( ubyte ptr, img + 4 )
#endif

	while( jinfo.output_scanline < jinfo.output_height )
		jpeg_read_scanlines( @jinfo, rows, 1 )

		select case( jinfo.out_color_space )
		case JCS_GRAYSCALE
			dim i as integer
			select case( bpp )
			case 24, 32
				for i = 0 to rowbytes-1
					'' Each i is within the row allocation. FB-LINTER: DISABLE-NEXT-LINE FBL525
					*cptr( ulong ptr, dst ) = rgb( src[i], src[i], src[i] )
					dst += 4
				next
			case 15, 16
				for i = 0 to rowbytes-1
					'' Each i is within the row allocation. FB-LINTER: DISABLE-NEXT-LINE FBL525
					pset img, (i, jinfo.output_scanline-1), rgb( src[i], src[i], src[i] )
				next
			case else
				'' 8 bpp and less require a proper global palette,
				'' which contains the colors used in the image
				'for i = 0 to rowbytes-1
				'	pset img, (i, jinfo.output_scanline-1), src[i]
				'next
				memcpy( dst, src, rowbytes )
				dst += pitch
			end select

		case JCS_RGB
			imageconvertrow( src, 24, dst, bpp, jinfo.output_width )
			dst += pitch
		end select
	wend

	jpeg_finish_decompress( @jinfo )
	function = img
	img = NULL
	imageread_jpg_cleanup( fp, jinfo, decoder_created, rows, img )
end function

'' end of example.bas
