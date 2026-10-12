/*
    Project: FreeBASIC PDFlib native qualification
    File: pdflib-tiff-native.c
    Purpose: Check the bundled TIFF file layouts and genuine decoder output.
    Responsibilities: Fixed-width file fields, odd-width rows and guarded pixels.
    This file contains no image encoder or replacement TIFF implementation.
*/
#include <tiffio.h>
#include <stdio.h>
#include <string.h>

_Static_assert(sizeof(uint32) == 4, "TIFF uint32 must occupy four bytes");
_Static_assert(sizeof(int32) == 4, "TIFF int32 must occupy four bytes");
_Static_assert(sizeof(TIFFHeader) == 8, "TIFF file header must occupy eight bytes");
_Static_assert(sizeof(TIFFDirEntry) == 12, "TIFF directory entries must occupy twelve bytes");

int main(int argc, char **argv)
{
    TIFF *image;
    uint32 width = 0, height = 0;
    uint16 channels = 0, bits = 0;
    unsigned char row[16+17*3+16];
    uint32 pixels[4+17*9+4];
    int result = 1;
    if (argc != 2) return 1;
    /* This is PDFlib's bundled API. Null allocator/error hooks select its
     * native CRT defaults; no unrelated system TIFF API is linked here. */
    image = TIFFOpen(argv[1], "r", NULL, NULL, NULL, NULL, NULL, NULL);
    if (!image) return 1;
    if (!TIFFGetField(image, TIFFTAG_IMAGEWIDTH, &width) ||
        !TIFFGetField(image, TIFFTAG_IMAGELENGTH, &height) ||
        !TIFFGetField(image, TIFFTAG_SAMPLESPERPIXEL, &channels) ||
        !TIFFGetField(image, TIFFTAG_BITSPERSAMPLE, &bits) ||
        width != 17 || height != 9 || channels != 3 || bits != 8 ||
        TIFFScanlineSize(image) != 51) goto cleanup;
    for (uint32 y = 0; y < height; ++y) {
        memset(row, 0xa5, sizeof(row));
        if (TIFFReadScanline(image, row+16, y, 0) != 1) goto cleanup;
        for (unsigned i = 0; i < 16; ++i)
            if (row[i] != 0xa5 || row[16+51+i] != 0xa5) goto cleanup;
        for (uint32 x = 0; x < width; ++x)
            if (row[16+x*3] != (unsigned char)(x*11+y*19) ||
                row[16+x*3+1] != (unsigned char)(x*7+y*37) ||
                row[16+x*3+2] != (unsigned char)(x*29+y*3)) goto cleanup;
    }
    memset(pixels, 0xa5, sizeof(pixels));
    if (!TIFFReadRGBAImage(image, width, height, pixels+4, 1)) goto cleanup;
    for (unsigned i = 0; i < 4; ++i)
        if (pixels[i] != 0xa5a5a5a5U || pixels[4+17*9+i] != 0xa5a5a5a5U) goto cleanup;
    /* The vendor API returns bottom-up packed ABGR pixels. */
    for (uint32 y = 0; y < height; ++y)
        for (uint32 x = 0; x < width; ++x) {
            uint32 value = pixels[4+(height-1-y)*width+x];
            if (TIFFGetR(value) != (unsigned char)(x*11+y*19) ||
                TIFFGetG(value) != (unsigned char)(x*7+y*37) ||
                TIFFGetB(value) != (unsigned char)(x*29+y*3)) goto cleanup;
        }
    puts("TIFF_NATIVE=PASS pixels=153 row_guard_bytes=32 header=8 directory=12");
    result = 0;
cleanup:
    TIFFClose(image);
    return result;
}
/* end of pdflib-tiff-native.c */
