/*
    Project: FreeBASIC native image qualification
    File: image-abi.c
    Purpose: Report the layouts used by the real image SDKs.
    Responsibilities: JPEG error/decode state, jump storage, FreeImage and GD.
    This file contains no replacement codec or guessed data layout.
*/
#include <stddef.h>
#include <stdio.h>
#include <setjmp.h>
#include <jpeglib.h>
#include <FreeImage.h>
#include <SDL/SDL_video.h>
#include <SDL/SDL_version.h>
#ifdef IMAGE_CHECK_GD
#include <gd.h>
#endif
int main(void)
{
    printf("JPEG %d %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu\n",
           JPEG_LIB_VERSION, sizeof(boolean), sizeof(struct jpeg_common_struct),
           sizeof(struct jpeg_decompress_struct), sizeof(struct jpeg_error_mgr),
           offsetof(struct jpeg_common_struct, client_data),
           offsetof(struct jpeg_decompress_struct, mem),
           offsetof(struct jpeg_decompress_struct, image_width),
           offsetof(struct jpeg_decompress_struct, image_height),
           offsetof(struct jpeg_decompress_struct, output_width),
           offsetof(struct jpeg_decompress_struct, output_height),
           offsetof(struct jpeg_decompress_struct, output_components),
           offsetof(struct jpeg_decompress_struct, output_scanline),
           offsetof(struct jpeg_decompress_struct, out_color_space),
           offsetof(struct jpeg_error_mgr, error_exit),
           offsetof(struct jpeg_error_mgr, msg_code), sizeof(jmp_buf));
    printf("JUMP_ALIGNMENT %zu\n", _Alignof(jmp_buf));
    printf("FREEIMAGE %zu %zu %zu %zu %zu %zu\n", sizeof(RGBQUAD), sizeof(BITMAPINFOHEADER),
           offsetof(BITMAPINFOHEADER, biWidth), offsetof(BITMAPINFOHEADER, biHeight),
           sizeof(FreeImageIO), offsetof(FreeImageIO, write_proc));
    printf("FREEIMAGE_VERSION %s\n", FreeImage_GetVersion());
    printf("SDL %zu %zu %zu %zu %zu %zu %zu %zu %zu\n", sizeof(SDL_Surface),
           offsetof(SDL_Surface, format), offsetof(SDL_Surface, w), offsetof(SDL_Surface, h),
           offsetof(SDL_Surface, pitch), offsetof(SDL_Surface, pixels), sizeof(SDL_PixelFormat),
           offsetof(SDL_PixelFormat, BitsPerPixel), offsetof(SDL_PixelFormat, Rmask));
    printf("SDL_VERSION %u.%u.%u\n", SDL_Linked_Version()->major,
           SDL_Linked_Version()->minor, SDL_Linked_Version()->patch);
#ifdef IMAGE_CHECK_GD
    printf("GD %zu %zu %zu %zu %zu\n", sizeof(gdImage), offsetof(gdImage, sx),
           offsetof(gdImage, sy), offsetof(gdImage, tpixels), offsetof(gdImage, interpolation));
    printf("GD_VERSION %s\n", gdVersionString());
#endif
    return 0;
}
/* end of image-abi.c */
