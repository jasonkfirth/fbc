/*
 * Project: FreeBASIC runtime library
 * File: drivers/haiku/haiku_runtime.h
 * Purpose: Declare Haiku-only console input helpers.
 * Responsibilities: Share focus and physical scancode interfaces.
 * This file intentionally does NOT contain: Input polling or event processing.
 */
#ifndef FB_HAIKU_RUNTIME_H
#define FB_HAIKU_RUNTIME_H

int fb_hConsoleHasFocus(void);
int fb_hInitScancodes(void);
unsigned char fb_hHaikuTranslateScancode(unsigned char code);

#endif

/* end of drivers/haiku/haiku_runtime.h */
