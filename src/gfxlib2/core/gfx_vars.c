/*
 * FreeBASIC gfxlib2: gfx_vars.c
 * Purpose: Define the common graphics state and transfer function pointers.
 * Responsibilities: driver selection, pages and mode queries.
 * This file intentionally does NOT contain drawing algorithms or native presentation.
 */

/* all global variables are defined here */

#include "../fb_gfx.h"

FBGFX *fb_gfx = NULL;
void *(*fb_hMemCpy)(void *dest, const void *src, size_t size) = memcpy;
void *(*fb_hMemSet)(void *dest, int value, size_t size) = memset;
void *(*fb_hPixelCpy)(void *dest, const void *src, size_t size) = NULL;
void *(*fb_hPixelSet)(void *dest, int color, size_t size) = NULL;
unsigned int *fb_color_conv_16to32 = NULL;
char *fb_window_title = NULL;
char *fb_gfx_driver_name = NULL;

/* end of gfx_vars.c */
