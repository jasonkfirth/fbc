
#ifndef DISABLE_HAIKU

#include "fb_gfx_haiku.h"
#include "haiku_window.h"
#include "../fb_gfx.h"

#include <View.h>

extern "C" void fb_hPostKey(int key);

/* ------------------------------------------------------------------------- */
/* Quit                                                                      */
/* ------------------------------------------------------------------------- */

void fb_hHaikuPostQuitEvent(void)
{
    fb_haiku.quitting = 1;

    /* match behaviour of other backends */
    fb_hPostKey(KEY_QUIT);
}


/* ------------------------------------------------------------------------- */
/* Keyboard                                                                  */
/* ------------------------------------------------------------------------- */

void fb_hHaikuHandleKeyDown(BView*, const char *bytes, int32 key)
{
    unsigned char sc = fb_hHaikuTranslateScancode((unsigned char)key);

    if (__fb_gfx && sc < 128)
        __fb_gfx->key[sc] = TRUE;

    if (bytes && bytes[0])
    {
        /* ASCII character */
        fb_hPostKey((unsigned char)bytes[0]);
    }
    else if (sc)
    {
        /* extended key */
        fb_hPostKey(sc);
    }
}


void fb_hHaikuHandleKeyUp(BView*, const char*, int32 key)
{
    unsigned char sc = fb_hHaikuTranslateScancode((unsigned char)key);

    if (__fb_gfx && sc < 128)
        __fb_gfx->key[sc] = FALSE;
}


/* ------------------------------------------------------------------------- */
/* Mouse                                                                     */
/* ------------------------------------------------------------------------- */

void fb_hHaikuHandleMouseMoved(BView*, int x, int y)
{
    fb_haiku.mouse_x = x;
    fb_haiku.mouse_y = y;
}


void fb_hHaikuHandleMouseDown(BView*, int x, int y, int buttons)
{
    fb_haiku.mouse_x = x;
    fb_haiku.mouse_y = y;

    /* buttons represents full button state */
    fb_haiku.mouse_buttons = buttons;
}


void fb_hHaikuHandleMouseUp(BView*, int x, int y, int buttons)
{
    fb_haiku.mouse_x = x;
    fb_haiku.mouse_y = y;

    /* buttons represents full button state after release */
    fb_haiku.mouse_buttons = buttons;
}

#endif
