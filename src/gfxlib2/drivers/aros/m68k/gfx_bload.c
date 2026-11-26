/*
    FreeBASIC gfxlib2 support for AROS m68k
    ---------------------------------------

    File: gfx_bload.c

    Purpose:

        Apply the m68k AROS BMP stream contract to the shared gfxlib2 BLOAD
        implementation.

    Responsibilities:

        - retain a probed file-signature byte without seeking an AROS volume
        - reuse the generic BMP validation and image-buffer implementation

    This file intentionally does NOT contain:

        - BMP header parsing or allocation logic
        - graphics presentation or Intuition window management
        - OMA game-specific image handling

    m68k AROS BMP stream contract:

        Mounted AROS streams cannot reliably rewind through fseek() after a
        format probe, so the first byte is returned through ungetc(). The
        shared decoder retains BMP's standard B, G, R order and packs the
        destination as 0xAARRGGBB on both little- and big-endian targets.
*/

#include <stdio.h>

#define FB_GFX_BLOAD_REWIND(file, first_byte) \
    (ungetc((first_byte), (file)) == EOF)

#include "../../../image/gfx_bload.c"

/* end of gfx_bload.c */
