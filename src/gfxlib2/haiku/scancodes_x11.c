#ifndef DISABLE_HAIKU

#include <stdint.h>

/*
    Haiku keycode -> PC/AT Set 1 scancode translation
*/

static uint8_t scancode_table[256];
static int scancode_initialized = 0;

static void fb_hInitScancodesInternal(void)
{
    int i;
    for(i=0;i<256;i++)
        scancode_table[i] = 0;

    /* ESC + function keys */
    scancode_table[0x01] = 1;    /* ESC */
    scancode_table[0x02] = 59;   /* F1 */
    scancode_table[0x03] = 60;   /* F2 */
    scancode_table[0x04] = 61;   /* F3 */
    scancode_table[0x05] = 62;   /* F4 */
    scancode_table[0x06] = 63;   /* F5 */
    scancode_table[0x07] = 64;   /* F6 */
    scancode_table[0x08] = 65;   /* F7 */
    scancode_table[0x09] = 66;   /* F8 */
    scancode_table[0x0a] = 67;   /* F9 */
    scancode_table[0x0b] = 68;   /* F10 */
    scancode_table[0x0c] = 87;   /* F11 */
    scancode_table[0x0d] = 88;   /* F12 */

    /* number row */
    scancode_table[0x11] = 41;   /* ` */
    scancode_table[0x12] = 2;    /* 1 */
    scancode_table[0x13] = 3;
    scancode_table[0x14] = 4;
    scancode_table[0x15] = 5;
    scancode_table[0x16] = 6;
    scancode_table[0x17] = 7;
    scancode_table[0x18] = 8;
    scancode_table[0x19] = 9;
    scancode_table[0x1a] = 10;
    scancode_table[0x1b] = 11;
    scancode_table[0x1c] = 12;   /* - */
    scancode_table[0x1d] = 13;   /* = */
    scancode_table[0x1e] = 14;   /* backspace */

    /* tab row */
    scancode_table[0x26] = 15;   /* TAB */
    scancode_table[0x27] = 16;   /* Q */
    scancode_table[0x28] = 17;   /* W */
    scancode_table[0x29] = 18;   /* E */
    scancode_table[0x2a] = 19;   /* R */
    scancode_table[0x2b] = 20;   /* T */
    scancode_table[0x2c] = 21;   /* Y */
    scancode_table[0x2d] = 22;   /* U */
    scancode_table[0x2e] = 23;   /* I */
    scancode_table[0x2f] = 24;   /* O */
    scancode_table[0x30] = 25;   /* P */
    scancode_table[0x31] = 26;   /* [ */
    scancode_table[0x32] = 27;   /* ] */
    scancode_table[0x33] = 43;   /* \ */

    /* home row */
    scancode_table[0x3b] = 58;   /* caps */
    scancode_table[0x3c] = 30;   /* A */
    scancode_table[0x3d] = 31;   /* S */
    scancode_table[0x3e] = 32;   /* D */
    scancode_table[0x3f] = 33;   /* F */
    scancode_table[0x40] = 34;   /* G */
    scancode_table[0x41] = 35;   /* H */
    scancode_table[0x42] = 36;   /* J */
    scancode_table[0x43] = 37;   /* K */
    scancode_table[0x44] = 38;   /* L */
    scancode_table[0x45] = 39;   /* ; */
    scancode_table[0x46] = 40;   /* ' */
    scancode_table[0x32] = 28;   /* ENTER */

    /* bottom row */
    scancode_table[0x4b] = 42;   /* left shift */
    scancode_table[0x4c] = 44;   /* Z */
    scancode_table[0x4d] = 45;   /* X */
    scancode_table[0x4e] = 46;   /* C */
    scancode_table[0x4f] = 47;   /* V */
    scancode_table[0x50] = 48;   /* B */
    scancode_table[0x51] = 49;   /* N */
    scancode_table[0x52] = 50;   /* M */
    scancode_table[0x53] = 51;   /* , */
    scancode_table[0x54] = 52;   /* . */
    scancode_table[0x55] = 53;   /* / */
    scancode_table[0x56] = 54;   /* right shift */

    /* modifiers */
    scancode_table[0x5c] = 29;   /* ctrl */
    scancode_table[0x5d] = 56;   /* alt */
    scancode_table[0x5e] = 57;   /* space */

    /* arrows */
    scancode_table[0x57] = 72;   /* up */
    scancode_table[0x61] = 75;   /* left */
    scancode_table[0x62] = 80;   /* down */
    scancode_table[0x63] = 77;   /* right */

    scancode_initialized = 1;
}

int fb_hInitScancodes(void)
{
    fb_hInitScancodesInternal();
    return 0;
}

unsigned char fb_hHaikuTranslateScancode(unsigned char key)
{
    if(!scancode_initialized)
        fb_hInitScancodesInternal();

    return scancode_table[key];
}

#endif
