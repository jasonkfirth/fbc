/*
 * FreeBASIC gfxlib2: gfx_image_info.c
 * Purpose: Inspect legacy and current BASIC image headers.
 * Responsibilities: image storage, conversion and checked file operations.
 * This file intentionally does NOT contain native windows or input dispatch.
 */

/* function to get information about FB image buffers. */

#include "../fb_gfx.h"
#include <limits.h>
#include <stdint.h>

FBCALL int fb_GfxImageInfo
	(
		void *img,
		ssize_t *width,
		ssize_t *height,
		ssize_t *bpp,
		ssize_t *pitch,
		void **imgdata,
		ssize_t *size
	)
{
	PUT_HEADER *header;
	int bpp_, width_, height_, pitch_, headerSize_;
	size_t total_size;

	header = (PUT_HEADER *)img;

	if (!header || header->type == 0)
	{
		*width = *height = *bpp = *pitch = *size = -1;
		*imgdata = NULL;
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	}
	else if (header->type == PUT_HEADER_NEW) {
		/* The legacy result fields are signed even though the header is not. */
		if ((header->bpp < 0) ||
		    (header->width > (unsigned int)INT_MAX) ||
		    (header->height > (unsigned int)INT_MAX) ||
		    (header->pitch > (unsigned int)INT_MAX))
			goto invalid_header;
		bpp_        = header->bpp;
		width_      = (int)header->width;
		height_     = (int)header->height;
		pitch_      = (int)header->pitch;
		headerSize_ = sizeof(PUT_HEADER);
	}
	else {
		bpp_        = header->old.bpp;
		width_      = header->old.width;
		height_     = header->old.height;
		pitch_      = width_ * bpp_;
		headerSize_ = 4;
	}

	/* Bound the byte count before multiplying the untrusted header fields. */
	if ((pitch_ < 0) || (height_ < 0) ||
	    ((height_ > 0) &&
	     ((size_t)pitch_ > (((size_t)-1 - (size_t)headerSize_) /
	                        (size_t)height_))))
		goto invalid_header;
	total_size = (size_t)headerSize_ +
	             ((size_t)pitch_ * (size_t)height_);
	if (total_size > (size_t)PTRDIFF_MAX)
		goto invalid_header;

	*width   = width_;
	*height  = height_;
	*bpp     = bpp_;
	*pitch   = pitch_;
	*imgdata = (unsigned char*)img + headerSize_;
	*size    = (ssize_t)total_size;

	return fb_ErrorSetNum( FB_RTERROR_OK );

invalid_header:
	*width = *height = *bpp = *pitch = *size = -1;
	*imgdata = NULL;
	return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
}

FBCALL int fb_GfxImageInfo32
	(
		void *img,
		int *width,
		int *height,
		int *bpp,
		int *pitch,
		void **imgdata,
		int *size
	)
{
	ssize_t w, h, b, p, s;
	int ret = fb_GfxImageInfo( img, &w, &h, &b, &p, imgdata, &s );
	if ((ret == FB_RTERROR_OK) &&
	    ((w > INT_MAX) || (h > INT_MAX) || (b > INT_MAX) ||
	     (p > INT_MAX) || (s > INT_MAX))) {
		ret = fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
		w = h = b = p = s = -1;
		*imgdata = NULL;
	}
	*width = (int)w;
	*height = (int)h;
	*bpp = (int)b;
	*pitch = (int)p;
	*size = (int)s;
	return ret;
}

FBCALL int fb_GfxImageInfo64
	(
		void *img,
		long long *width,
		long long *height,
		long long *bpp,
		long long *pitch,
		void **imgdata,
		long long *size
	)
{
	ssize_t w, h, b, p, s;
	int ret = fb_GfxImageInfo( img, &w, &h, &b, &p, imgdata, &s );
	*width = (long long)w;
	*height = (long long)h;
	*bpp = (long long)b;
	*pitch = (long long)p;
	*size = (long long)s;
	return ret;
}

/* end of gfx_image_info.c */
