/*
    FreeBASIC native Allegro qualification
    File: allegro-glyph.c
    Purpose: Verify native glyph drawing on allocated bitmaps at every depth.
    Responsibilities: Pixel checks, pointer-width checks and bitmap ownership.
    This file contains no FreeBASIC bindings, input helper or window capture.
*/
#include <allegro.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>

int main(void)
{
    static const int depths[] = {8, 15, 16, 24, 32};
    unsigned int index;
    if(install_allegro(SYSTEM_NONE, &errno, atexit) != 0)
        return 1;
    for(index = 0; index < sizeof(depths) / sizeof(depths[0]); index++) {
        BITMAP *bitmap = create_bitmap_ex(depths[index], 128, 32);
        int x, y, colored = 0;
        int foreground = depths[index] == 8 ? 255 : makecol_depth(depths[index], 255, 255, 255);
        if(bitmap == NULL) {
            allegro_exit();
            return 2;
        }
        /* A low allocation could hide a Win64 pointer truncation. Native
           Windows qualification must exercise an address above 4 GiB. */
#if defined(_WIN64)
        if((uintptr_t)bitmap->line[0] <= UINT32_MAX) {
            destroy_bitmap(bitmap);
            allegro_exit();
            return 3;
        }
#endif
        clear_to_color(bitmap, 0);
        textout_ex(bitmap, font, "Native glyph", 4, 4, foreground, -1);
        for(y = 0; y < bitmap->h; y++) {
            for(x = 0; x < bitmap->w; x++) {
                int pixel = getpixel(bitmap, x, y);
                if(pixel == foreground)
                    colored++;
                else if(pixel != 0) {
                    destroy_bitmap(bitmap);
                    allegro_exit();
                    return 4;
                }
            }
        }
        if(colored <= 0 || colored >= bitmap->w * bitmap->h) {
            destroy_bitmap(bitmap);
            allegro_exit();
            return 5;
        }
        printf("NATIVE_GLYPH depth=%d pixels=%d address=%p\n", depths[index], colored, (void *)bitmap->line[0]);
        fflush(stdout);
        triangle(bitmap, 16, 4, 8, 12, 24, 12, foreground);
        if(getpixel(bitmap, 16, 8) != foreground) {
            destroy_bitmap(bitmap);
            allegro_exit();
            return 6;
        }
        destroy_bitmap(bitmap);
    }
    allegro_exit();
    puts("ALLEGRO_NATIVE_GLYPH=PASS");
    return 0;
}
END_OF_MAIN()
/* end of allegro-glyph.c */
