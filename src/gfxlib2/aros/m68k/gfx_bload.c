/*
    FreeBASIC gfxlib2 support for AROS m68k
    ---------------------------------------

    File: gfx_bload.c

    Purpose:

        Apply the m68k AROS BMP stream and 24-bit pixel byte-lane contract to
        the shared gfxlib2 BLOAD implementation.

    Responsibilities:

        - retain a probed file-signature byte without seeking an AROS volume
        - normalize m68k AROS RGB bytes before generic BMP conversion
        - reuse the generic BMP validation and image-buffer implementation

    This file intentionally does NOT contain:

        - BMP header parsing or allocation logic
        - graphics presentation or Intuition window management
        - OMA game-specific image handling

    m68k AROS BMP contract:

        Mounted AROS streams cannot reliably rewind through fseek() after a
        format probe, so the first byte is returned through ungetc().  The
        AROS m68k C runtime presents the corpus' RGB triplets through rotated
        32-bit conversion lanes.  This wrapper restores the R, G, B order
        before the shared decoder stores 0xAARRGGBB framebuffer pixels.
*/

#include <stdint.h>
#include <stdio.h>

#define FB_GFX_BMP24_PACK(source) \
    (((uint32_t)(source)[1]) | ((uint32_t)(source)[2] << 8) | \
     ((uint32_t)(source)[0] << 16))

#define FB_GFX_BLOAD_REWIND(file, first_byte) \
    (ungetc((first_byte), (file)) == EOF)

#include "../../gfx_bload.c"

/* end of gfx_bload.c */
