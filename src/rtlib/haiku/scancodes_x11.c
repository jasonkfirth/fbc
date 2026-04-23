/*
    FreeBASIC Haiku backend
    --------------------------------

    File: scancodes_x11.c

    Purpose:

        Provide translation from platform keycodes into the PC-style
        keyboard scancodes expected by the FreeBASIC graphics runtime.

    Responsibilities:

        • maintain a translation table for keycodes
        • initialize keyboard state used by the runtime
        • convert platform keycodes into PC scancodes used by MULTIKEY()

    This file intentionally does NOT contain:

        • platform event handling
        • keyboard input polling
        • window management

    Notes:

        The FreeBASIC graphics runtime expects IBM PC set-1 keyboard
        scancodes for functions such as MULTIKEY().

        Platform event systems typically provide different keycode
        values, so a translation table is required.

        This file provides a simple lookup table mapping common
        keycodes to their PC equivalents.
*/

#ifndef DISABLE_HAIKU

#include <string.h>


/* ------------------------------------------------------------------------- */
/* Translation table                                                         */
/* ------------------------------------------------------------------------- */

/*
    Table mapping platform keycodes to PC scancodes.

    Entries not explicitly defined remain zero, which represents
    an unmapped key.
*/

static unsigned char haiku_to_pc_scancode[256];


/* ------------------------------------------------------------------------- */
/* Table initialization                                                      */
/* ------------------------------------------------------------------------- */

static void fb_hHaikuInitScancodeTable(void)
{
    memset(haiku_to_pc_scancode, 0, sizeof(haiku_to_pc_scancode));


    /* ------------------------------------------------------------- */
    /* Alphabet keys                                                 */
    /* ------------------------------------------------------------- */

    haiku_to_pc_scancode['a'] = 0x1E;
    haiku_to_pc_scancode['b'] = 0x30;
    haiku_to_pc_scancode['c'] = 0x2E;
    haiku_to_pc_scancode['d'] = 0x20;
    haiku_to_pc_scancode['e'] = 0x12;
    haiku_to_pc_scancode['f'] = 0x21;
    haiku_to_pc_scancode['g'] = 0x22;
    haiku_to_pc_scancode['h'] = 0x23;
    haiku_to_pc_scancode['i'] = 0x17;
    haiku_to_pc_scancode['j'] = 0x24;
    haiku_to_pc_scancode['k'] = 0x25;
    haiku_to_pc_scancode['l'] = 0x26;
    haiku_to_pc_scancode['m'] = 0x32;
    haiku_to_pc_scancode['n'] = 0x31;
    haiku_to_pc_scancode['o'] = 0x18;
    haiku_to_pc_scancode['p'] = 0x19;
    haiku_to_pc_scancode['q'] = 0x10;
    haiku_to_pc_scancode['r'] = 0x13;
    haiku_to_pc_scancode['s'] = 0x1F;
    haiku_to_pc_scancode['t'] = 0x14;
    haiku_to_pc_scancode['u'] = 0x16;
    haiku_to_pc_scancode['v'] = 0x2F;
    haiku_to_pc_scancode['w'] = 0x11;
    haiku_to_pc_scancode['x'] = 0x2D;
    haiku_to_pc_scancode['y'] = 0x15;
    haiku_to_pc_scancode['z'] = 0x2C;


    /* ------------------------------------------------------------- */
    /* Numeric keys                                                  */
    /* ------------------------------------------------------------- */

    haiku_to_pc_scancode['1'] = 0x02;
    haiku_to_pc_scancode['2'] = 0x03;
    haiku_to_pc_scancode['3'] = 0x04;
    haiku_to_pc_scancode['4'] = 0x05;
    haiku_to_pc_scancode['5'] = 0x06;
    haiku_to_pc_scancode['6'] = 0x07;
    haiku_to_pc_scancode['7'] = 0x08;
    haiku_to_pc_scancode['8'] = 0x09;
    haiku_to_pc_scancode['9'] = 0x0A;
    haiku_to_pc_scancode['0'] = 0x0B;


    /* ------------------------------------------------------------- */
    /* Control keys                                                  */
    /* ------------------------------------------------------------- */

    haiku_to_pc_scancode[' ']  = 0x39; /* space */
    haiku_to_pc_scancode['\n'] = 0x1C; /* enter */
    haiku_to_pc_scancode['\t'] = 0x0F; /* tab */
    haiku_to_pc_scancode['\b'] = 0x0E; /* backspace */


    /* ------------------------------------------------------------- */
    /* Cursor keys (common Haiku keycodes)                           */
    /* ------------------------------------------------------------- */

    haiku_to_pc_scancode[0x61] = 0x4B; /* left  */
    haiku_to_pc_scancode[0x63] = 0x4D; /* right */
    haiku_to_pc_scancode[0x57] = 0x48; /* up    */
    haiku_to_pc_scancode[0x62] = 0x50; /* down  */


    /* ------------------------------------------------------------- */
    /* Escape key                                                    */
    /* ------------------------------------------------------------- */

    haiku_to_pc_scancode[0x01] = 0x01;


    /* ------------------------------------------------------------- */
    /* Function keys                                                 */
    /* ------------------------------------------------------------- */

    haiku_to_pc_scancode[0x02] = 0x3B; /* F1  */
    haiku_to_pc_scancode[0x03] = 0x3C; /* F2  */
    haiku_to_pc_scancode[0x04] = 0x3D; /* F3  */
    haiku_to_pc_scancode[0x05] = 0x3E; /* F4  */
    haiku_to_pc_scancode[0x06] = 0x3F; /* F5  */
    haiku_to_pc_scancode[0x07] = 0x40; /* F6  */
    haiku_to_pc_scancode[0x08] = 0x41; /* F7  */
    haiku_to_pc_scancode[0x09] = 0x42; /* F8  */
    haiku_to_pc_scancode[0x0A] = 0x43; /* F9  */
    haiku_to_pc_scancode[0x0B] = 0x44; /* F10 */
}


/* ------------------------------------------------------------------------- */
/* Runtime initialization                                                    */
/* ------------------------------------------------------------------------- */

int fb_hInitScancodes(void)
{

    fb_hHaikuInitScancodeTable();

    return 0;
}


/* ------------------------------------------------------------------------- */
/* Translation function                                                      */
/* ------------------------------------------------------------------------- */

/*
    Convert platform keycode to PC scancode.
*/

unsigned char fb_hHaikuTranslateScancode(unsigned char code)
{
    return haiku_to_pc_scancode[code];
}

#endif

/* end of scancodes_x11.c */
