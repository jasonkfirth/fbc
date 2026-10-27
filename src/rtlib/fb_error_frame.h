/*
 * Project: FreeBASIC runtime library
 * File: fb_error_frame.h
 * Purpose: Define C-backend error activation and callback contracts.
 * Responsibilities: Keep handler ownership separate from legacy jump addresses
 *     and expose caller-owned activation storage without extending FB_ERRORCTX.
 * This file intentionally does NOT contain: compiler lowering or handler bodies.
 */
#ifndef FB_ERROR_FRAME_H
#define FB_ERROR_FRAME_H

typedef int (*FB_ERROR_FRAME_CALLBACK)( void *context );
typedef void (*FB_ERROR_FRAME_CLEANUP)( void *context );

enum {
	FB_ERROR_FRAME_UNHANDLED,
	FB_ERROR_FRAME_RETRY,
	FB_ERROR_FRAME_NEXT,
	FB_ERROR_FRAME_TRANSFER
};

/* The compiler keeps this storage alive for the complete C activation. The
 * fields are initialized before its owner checkpoint and remain unchanged
 * until leave. State modified after setjmp belongs to the allocated owner. */
typedef struct FB_ERROR_FRAME {
	struct FB_ERROR_FRAME *previous;
	void *context;
	FB_ERROR_FRAME_CLEANUP cleanup;
	void *owner;
} FB_ERROR_FRAME;

#ifdef __cplusplus
extern "C" {
#endif

FBCALL int   fb_ErrorFrameEnter( void *storage, void *context, FB_ERROR_FRAME_CLEANUP cleanup );
FBCALL void *fb_ErrorFrameBuffer( void *storage );
FBCALL int   fb_ErrorFrameReady( void *storage );
FBCALL int   fb_ErrorFrameSetHandler( void *storage, FB_ERROR_FRAME_CALLBACK callback, void **previous );
FBCALL void *fb_ErrorFrameRestore( void *handler );
FBCALL int   fb_ErrorFrameSetTarget( void *storage, int target );
FBCALL int   fb_ErrorFrameGetTarget( void *storage );
FBCALL int   fb_ErrorFrameLeave( void *storage );

/* Internal bridge used by the legacy throw/resume entry points. Dispatch
 * returns zero for a legacy label, one for a same-activation continuation,
 * and minus one for an abort whose error number is written to fatal_error. */
void fb_ErrorFrameSaveState( FB_ERRORCTX *error );
void fb_ErrorFrameRestoreState( FB_ERRORCTX *error );
int fb_ErrorFrameDispatch( FB_ERRORCTX *error, FB_ERRHANDLER *target, int *fatal_error );
void fb_ERRORFRAMECTX_Destructor( void *context );

#ifdef __cplusplus
}
#endif

#endif
/* end of fb_error_frame.h */
