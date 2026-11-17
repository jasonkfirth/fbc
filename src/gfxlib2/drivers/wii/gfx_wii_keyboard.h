/*
 * Project: FreeBASIC Wii graphics input
 * File: gfx_wii_keyboard.h
 * Purpose: Keep the IOS keyboard adapter separate from framebuffer rendering.
 * Responsibilities: Graphics-owned keyboard lifetime, polling and held state.
 * This file does not define public runtime APIs or platform implementations.
 */
#ifndef FB_GFX_WII_KEYBOARD_H
#define FB_GFX_WII_KEYBOARD_H
void fb_hWiiKeyboardInit( void );
void fb_hWiiKeyboardExit( void );
void fb_hWiiKeyboardPoll( void );
int fb_hWiiKeyboardDown( int scancode );
#endif
/* end of gfx_wii_keyboard.h */
