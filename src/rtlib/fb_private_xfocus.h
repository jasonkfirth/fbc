/*
 * Project: FreeBASIC Unix runtime
 * File: fb_private_xfocus.h
 * Purpose: Share the XTerm focus helper contract across Unix input drivers.
 * Responsibilities: Declare balanced initialization, queries and release.
 * Dynamic X11 loading and display ownership belong to drivers/unix/io_xfocus.c.
 */
#ifndef FB_PRIVATE_XFOCUS_H
#define FB_PRIVATE_XFOCUS_H

/* Keyboard and mouse callers serialize access with the console background
   lock. Each initialization attempt owns one matching release, including an
   attempt that reports unavailable X11. Queries then return false until a
   later initialization succeeds. */
int fb_hXTermInitFocus(void);
void fb_hXTermExitFocus(void);
int fb_hXTermHasFocus(void);

#endif

/* end of fb_private_xfocus.h */
