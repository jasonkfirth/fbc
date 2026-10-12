/*
    FreeBASIC native library regression checks
    File: allegro-abi.c
    Purpose: Obtain classic Allegro layouts from the native C headers.
    Responsibilities: Bitmap prefix, dialog fields and platform-sized values.
    This file contains no Allegro 5 compatibility layer or replacement renderer.
*/
#include <stddef.h>
#include <stdio.h>
#include <allegro.h>

int main(void)
{
    /* BITMAP ends in a flexible scanline array. Compare the prefix and its
       array offset rather than treating one FreeBASIC pointer as C storage. */
    printf("ABI %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu\n",
           sizeof(RGB), sizeof(DIALOG), offsetof(DIALOG, dp),
           sizeof(SAMPLE), offsetof(SAMPLE, len), offsetof(SAMPLE, data),
           sizeof(FONT), offsetof(FONT, vtable), offsetof(BITMAP, line),
           offsetof(BITMAP, id), sizeof(((BITMAP *)0)->id), sizeof(GFX_MODE));
    printf("VERSION %s\n", allegro_id);
    return 0;
}
END_OF_MAIN()
/* end of allegro-abi.c */
