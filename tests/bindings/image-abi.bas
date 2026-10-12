'' FreeBASIC native image qualification
'' File: image-abi.bas
'' Purpose: Compare the binding's used fields with the actual C SDK.
'' Responsibilities: JPEG/FreeImage/GD layouts and jump-buffer storage.
'' This file contains no replacement codec or guessed native layout.
#lang "fb"
#include once "jpeglib.bi"
#include once "crt/setjmp.bi"
#include once "FreeImage.bi"
#include once "SDL/SDL.bi"
#ifdef IMAGE_CHECK_GD
#include once "gd.bi"
#endif
Print "JPEG"; JPEG_LIB_VERSION; SizeOf(jpeg_boolean); SizeOf(jpeg_common_struct); _
	SizeOf(jpeg_decompress_struct); SizeOf(jpeg_error_mgr); OffsetOf(jpeg_common_struct, client_data); _
	OffsetOf(jpeg_decompress_struct, mem); OffsetOf(jpeg_decompress_struct, image_width); _
	OffsetOf(jpeg_decompress_struct, image_height); OffsetOf(jpeg_decompress_struct, output_width); _
	OffsetOf(jpeg_decompress_struct, output_height); OffsetOf(jpeg_decompress_struct, output_components); _
	OffsetOf(jpeg_decompress_struct, output_scanline); OffsetOf(jpeg_decompress_struct, out_color_space); _
	OffsetOf(jpeg_error_mgr, error_exit); OffsetOf(jpeg_error_mgr, msg_code); SizeOf(jmp_buf)
Print "FREEIMAGE"; SizeOf(RGBQUAD); SizeOf(BITMAPINFOHEADER); OffsetOf(BITMAPINFOHEADER, biWidth); _
	OffsetOf(BITMAPINFOHEADER, biHeight); SizeOf(FreeImageIO); OffsetOf(FreeImageIO, write_proc)
Print "FREEIMAGE_VERSION "; *FreeImage_GetVersion()
Print "SDL"; SizeOf(SDL_Surface); OffsetOf(SDL_Surface, format); OffsetOf(SDL_Surface, w); _
	OffsetOf(SDL_Surface, h); OffsetOf(SDL_Surface, pitch); OffsetOf(SDL_Surface, pixels); _
	SizeOf(SDL_PixelFormat); OffsetOf(SDL_PixelFormat, BitsPerPixel); OffsetOf(SDL_PixelFormat, Rmask)
#ifdef IMAGE_CHECK_GD
Print "GD"; SizeOf(gdImage); OffsetOf(gdImage, sx); OffsetOf(gdImage, sy); _
	OffsetOf(gdImage, tpixels); OffsetOf(gdImage, interpolation)
Print "GD_VERSION "; *gdVersionString()
#endif
'' end of image-abi.bas
