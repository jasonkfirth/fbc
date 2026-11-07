/*
 * Project: FreeBASIC runtime
 * File: fb_gosub.h
 * Purpose: Declare the compiler-facing GOSUB context protocol.
 * Responsibilities: Keep the one-pointer context ABI and both continuation modes.
 * This file intentionally does NOT contain: node or jump-buffer layouts.
 * Include fb.h before this internal runtime header.
 */
#ifndef FB_GOSUB_H
#define FB_GOSUB_H

FBCALL void *fb_GosubPush( void **context );
FBCALL int fb_GosubPop( void **context );
FBCALL int fb_GosubReturn( void **context );
FBCALL void fb_GosubExit( void **context );
FBCALL int fb_GosubPushTarget( void **context, unsigned int target );
FBCALL unsigned int fb_GosubReturnTarget( void **context );

#endif
/* end of fb_gosub.h */
