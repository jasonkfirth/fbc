/*
 * Project: FreeBASIC private C resume bridge
 * File: drivers/win32/x86_64/error_c_resume_internal.h
 * Purpose: Declare the activation-local resume experiment.
 * Responsibilities: Keep jump buffers opaque to emitted C.
 * This file intentionally does NOT contain: public error-context layout changes.
 */
#ifndef FB_ERROR_C_RESUME_INTERNAL_H
#define FB_ERROR_C_RESUME_INTERNAL_H
FBCALL void *fb_ErrorCResumePush( void *retry, void *next );
FBCALL void *fb_ErrorCResumeEnter( void *frame, const void *retry, const void *next );
FBCALL void *fb_ErrorCResumeTarget( void *buffer );
FBCALL int fb_ErrorCResumePop( void *buffer );
int fb_ErrorCResumeToken( void *token );
void fb_ErrorNativeResumeState( void *label );
FBCALL FB_ERRHANDLER fb_ErrorSetHandlerRaw( FB_ERRHANDLER handler );
FBCALL FB_ERRHANDLER fb_ErrorSetNativeHandler( FB_ERRHANDLER handler, void *frame );
FBCALL FB_ERRHANDLER fb_ErrorSetNativeHandlerSized( FB_ERRHANDLER handler, void *frame, size_t outgoing_space );
FBCALL FB_ERRHANDLER fb_ErrorSetNativeHandlerLegacy( FB_ERRHANDLER handler, void *frame, void *stack );
FBCALL void fb_ErrorNativeDispatch( void *target ) __attribute__((noreturn));
FBCALL void fb_ErrorNativeDispatchRaw( void *target, void *frame, size_t outgoing_space ) __attribute__((noreturn));
FBCALL void *fb_ErrorNativeHandlerFrame( void *handler );
FBCALL FB_ERRHANDLER fb_ErrorNativeHandlerTarget( FB_ERRHANDLER handler );
FBCALL int fb_ErrorNativeOwnerCleanup( void *frame );
FBCALL void fb_ErrorNativeOwnerCleanupChecked( void *frame );
FBCALL void fb_ErrorNativeOwnerCleanupCurrent( void );
FBCALL int fb_ErrorNativeOwnerLeave( void *frame );
FBCALL int fb_ErrorNativeOwnerLeaveChecked( void *frame );
FBCALL int fb_ErrorNativeOwnerLeaveCurrent( void );
#endif
/* end of drivers/win32/x86_64/error_c_resume_internal.h */
