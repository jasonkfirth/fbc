/*
 * Project: FreeBASIC runtime library
 * File: errors/error_frame.c
 * Purpose: Dispatch C error callbacks without jumping between C functions.
 * Responsibilities: Own callback metadata, preserve nested error state, and
 *     clean up live child activations before a one-way owner transfer.
 * This file intentionally does NOT contain: BASIC outlining or caller storage.
 */
#include "fb.h"
#include <setjmp.h>

#if defined( _WIN32 ) && defined( __aarch64__ )
void __mingw_longjmp( jmp_buf buffer, int value );
#endif

/* ------------------------------------------------------------------------- */
/* Activation ownership and callback metadata                                 */
/* ------------------------------------------------------------------------- */

typedef struct FB_ERROR_OWNER FB_ERROR_OWNER;
typedef struct FB_ERROR_CALLBACK FB_ERROR_CALLBACK;

struct FB_ERROR_CALLBACK {
	FB_ERROR_CALLBACK *next;
	FB_ERROR_OWNER *owner;
	FB_ERROR_FRAME_CALLBACK callback;
};

struct FB_ERROR_OWNER {
	jmp_buf buffer;
	FB_ERROR_OWNER *next;
	FB_ERROR_FRAME *frame;
	FB_ERROR_CALLBACK *handlers;
	void *context;
	FB_ERRHANDLER previous_handler;
	int target;
	unsigned flags;
};

typedef struct FB_ERROR_DISPATCH {
	struct FB_ERROR_DISPATCH *previous;
	FB_ERROR_CALLBACK *handler;
	FB_ERROR_FRAME *anchor;
	FB_ERRORCTX error;
} FB_ERROR_DISPATCH;

typedef struct FB_ERRORFRAMECTX {
	FB_ERROR_FRAME *top;
	FB_ERROR_OWNER *owners;
	FB_ERROR_DISPATCH *dispatch;
	FB_ERROR_FRAME *unwinding;
} FB_ERRORFRAMECTX;

enum {
	FB_ERROR_OWNER_READY = 1,
	FB_ERROR_OWNER_ACTIVATED = 2
};

/* The historical error ABI transports label addresses using FB_ERRHANDLER.
 * Supported targets already require equal code/data address widths. Copy the
 * representation explicitly; callback records are never called as code. */
typedef char FB_ERROR_HANDLER_ADDRESS_WIDTH[(sizeof( FB_ERRHANDLER ) == sizeof( void * )) ? 1 : -1];

static FB_ERRHANDLER hLegacyAddress( void *address )
{
	FB_ERRHANDLER result;
	memcpy( &result, &address, sizeof( result ) );
	return result;
}

static void *hDataAddress( FB_ERRHANDLER address )
{
	void *result;
	memcpy( &result, &address, sizeof( result ) );
	return result;
}

/* ------------------------------------------------------------------------- */
/* Registration, retirement and TLS lifetime                                   */
/* ------------------------------------------------------------------------- */

/* Only activation entry creates this context. Legacy throw/resume hooks must
 * stay allocation-free when no new C owner has been registered. */
static FB_ERRORFRAMECTX *hGetContext( void )
{
	return (FB_ERRORFRAMECTX *)fb_TlsGetCtxExisting( FB_TLSKEY_ERRORFRAME );
}

static FB_ERROR_FRAME *hFindFrame( FB_ERRORFRAMECTX *context, const void *storage )
{
	FB_ERROR_FRAME *frame = context->top;
	while( frame != NULL ) {
		if( frame == storage )
			return frame;
		frame = frame->previous;
	}
	return NULL;
}

static FB_ERROR_CALLBACK *hFindHandler( FB_ERRORFRAMECTX *context, FB_ERRHANDLER token )
{
	FB_ERROR_OWNER *owner;
	FB_ERROR_CALLBACK *handler;
	void *address = hDataAddress( token );

	for( owner = context->owners; owner != NULL; owner = owner->next ) {
		for( handler = owner->handlers; handler != NULL; handler = handler->next ) {
			if( handler == address )
				return handler;
		}
	}
	return NULL;
}

static void hFreeOwner( FB_ERROR_OWNER *owner )
{
	FB_ERROR_CALLBACK *handler = owner->handlers;
	while( handler != NULL ) {
		FB_ERROR_CALLBACK *next = handler->next;
		free( handler );
		handler = next;
	}
	free( owner );
}

void fb_ERRORFRAMECTX_Destructor( void *opaque )
{
	FB_ERRORFRAMECTX *context = (FB_ERRORFRAMECTX *)opaque;
	FB_ERROR_OWNER *owner = context->owners;

	/* TLS teardown may run after caller activations have disappeared. Release
	 * only owned metadata; never dereference their borrowed frame/context data. */
	while( owner != NULL ) {
		FB_ERROR_OWNER *next = owner->next;
		hFreeOwner( owner );
		owner = next;
	}
	context->owners = NULL;
	context->top = NULL;
	context->dispatch = NULL;
	context->unwinding = NULL;
}

FBCALL int fb_ErrorFrameEnter( void *storage, void *data, FB_ERROR_FRAME_CLEANUP cleanup )
{
	FB_ERRORFRAMECTX *context = FB_TLSGETCTX( ERRORFRAME );
	FB_ERROR_FRAME *frame = (FB_ERROR_FRAME *)storage;

	if( context == NULL )
		return FB_RTERROR_OUTOFMEM;
	if( frame == NULL || hFindFrame( context, frame ) != NULL )
		return FB_RTERROR_ILLEGALFUNCTIONCALL;
	frame->previous = context->top;
	frame->context = data;
	frame->cleanup = cleanup;
	frame->owner = NULL;
	context->top = frame;
	return FB_RTERROR_OK;
}

FBCALL void *fb_ErrorFrameBuffer( void *storage )
{
	FB_ERRORFRAMECTX *context = hGetContext();
	FB_ERROR_FRAME *frame;
	FB_ERROR_OWNER *owner;

	if( context == NULL || (frame = hFindFrame( context, storage )) == NULL || context->unwinding != NULL )
		return NULL;
	owner = (FB_ERROR_OWNER *)frame->owner;
	if( owner == NULL ) {
		owner = (FB_ERROR_OWNER *)calloc( 1, sizeof( *owner ) );
		if( owner == NULL ) {
			fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
			return NULL;
		}
		owner->frame = frame;
		owner->context = frame->context;
		owner->next = context->owners;
		context->owners = owner;
		frame->owner = owner;
	}
	return &owner->buffer;
}

FBCALL int fb_ErrorFrameReady( void *storage )
{
	FB_ERRORFRAMECTX *context = hGetContext();
	FB_ERROR_FRAME *frame;

	if( context == NULL || (frame = hFindFrame( context, storage )) == NULL || frame->owner == NULL ||
	    context->unwinding != NULL )
		return FB_RTERROR_ILLEGALFUNCTIONCALL;
	((FB_ERROR_OWNER *)frame->owner)->flags |= FB_ERROR_OWNER_READY;
	return FB_RTERROR_OK;
}

FBCALL int fb_ErrorFrameSetHandler( void *storage, FB_ERROR_FRAME_CALLBACK callback, void **old_handler )
{
	FB_ERRORFRAMECTX *context = hGetContext();
	FB_ERROR_FRAME *frame;
	FB_ERROR_OWNER *owner;
	FB_ERROR_CALLBACK *handler;
	FB_ERRORCTX *error;
	FB_ERRHANDLER previous;

	if( context == NULL )
		return FB_RTERROR_OUTOFMEM;
	if( (frame = hFindFrame( context, storage )) == NULL || frame->owner == NULL ||
	    callback == NULL || old_handler == NULL || context->unwinding != NULL )
		return FB_RTERROR_ILLEGALFUNCTIONCALL;
	owner = (FB_ERROR_OWNER *)frame->owner;
	for( handler = owner->handlers; handler != NULL; handler = handler->next ) {
		if( handler->callback == callback )
			break;
	}
	if( handler == NULL ) {
		handler = (FB_ERROR_CALLBACK *)calloc( 1, sizeof( *handler ) );
		if( handler == NULL ) {
			return FB_RTERROR_OUTOFMEM;
		}
		handler->owner = owner;
		handler->callback = callback;
		handler->next = owner->handlers;
		owner->handlers = handler;
	}
	error = FB_TLSGETCTX( ERROR );
	if( error == NULL )
		return FB_RTERROR_OUTOFMEM;
	previous = error->handler;
	if( (owner->flags & FB_ERROR_OWNER_ACTIVATED) == 0 ) {
		owner->previous_handler = previous;
		owner->flags |= FB_ERROR_OWNER_ACTIVATED;
	}
	error->handler = hLegacyAddress( handler );
	*old_handler = hDataAddress( previous );
	return FB_RTERROR_OK;
}

FBCALL void *fb_ErrorFrameRestore( void *handler )
{
	return hDataAddress( fb_ErrorSetHandler( hLegacyAddress( handler ) ) );
}

FBCALL int fb_ErrorFrameSetTarget( void *storage, int target )
{
	FB_ERRORFRAMECTX *context = hGetContext();
	FB_ERROR_FRAME *frame;

	if( context == NULL || (frame = hFindFrame( context, storage )) == NULL || frame->owner == NULL ||
	    context->dispatch == NULL || context->dispatch->handler->owner->frame != frame || target <= 0 ||
	    context->unwinding != NULL )
		return FB_RTERROR_ILLEGALFUNCTIONCALL;
	((FB_ERROR_OWNER *)frame->owner)->target = target;
	return FB_RTERROR_OK;
}

FBCALL int fb_ErrorFrameGetTarget( void *storage )
{
	FB_ERRORFRAMECTX *context = hGetContext();
	FB_ERROR_FRAME *frame;
	if( context == NULL || (frame = hFindFrame( context, storage )) == NULL || frame->owner == NULL )
		return 0;
	return ((FB_ERROR_OWNER *)frame->owner)->target;
}

static void hPopFrame( FB_ERRORFRAMECTX *context, FB_ERRORCTX *error )
{
	FB_ERROR_FRAME *frame = context->top;
	FB_ERROR_OWNER *owner = (FB_ERROR_OWNER *)frame->owner;
	FB_ERROR_OWNER **link = &context->owners;
	FB_ERROR_CALLBACK *handler = error != NULL ? hFindHandler( context, error->handler ) : NULL;

	if( owner != NULL ) {
		if( handler != NULL && handler->owner == owner )
			error->handler = owner->previous_handler;
		while( *link != NULL && *link != owner )
			link = &(*link)->next;
		if( *link == owner )
			*link = owner->next;
		hFreeOwner( owner );
	}
	context->top = frame->previous;
	frame->owner = NULL;
}

FBCALL int fb_ErrorFrameLeave( void *storage )
{
	FB_ERRORFRAMECTX *context = hGetContext();
	FB_ERROR_DISPATCH *dispatch;
	FB_ERRORCTX *error;

	if( context == NULL || context->top == NULL || context->top != storage ||
	    context->top == context->unwinding )
		return FB_RTERROR_ILLEGALFUNCTIONCALL;
	for( dispatch = context->dispatch; dispatch != NULL; dispatch = dispatch->previous ) {
		if( dispatch->anchor == storage || dispatch->handler->owner->frame == storage )
			return FB_RTERROR_ILLEGALFUNCTIONCALL;
	}
	error = (FB_ERRORCTX *)fb_TlsGetCtxExisting( FB_TLSKEY_ERROR );
	hPopFrame( context, error );
	return FB_RTERROR_OK;
}

/* ------------------------------------------------------------------------- */
/* Nested error state and one-way owner transfers                             */
/* ------------------------------------------------------------------------- */

void fb_ErrorFrameSaveState( FB_ERRORCTX *error )
{
	FB_ERRORFRAMECTX *context = hGetContext();
	if( context != NULL && context->dispatch != NULL )
		context->dispatch->error = *error;
}

void fb_ErrorFrameRestoreState( FB_ERRORCTX *error )
{
	FB_ERRORFRAMECTX *context = hGetContext();
	if( context != NULL && context->dispatch != NULL ) {
		FB_ERRHANDLER handler = error->handler;
		*error = context->dispatch->error;
		error->handler = handler;
	}
}

static int hFrameAtOrAbove( FB_ERRORFRAMECTX *context, FB_ERROR_FRAME *frame, FB_ERROR_FRAME *owner )
{
	FB_ERROR_FRAME *current = context->top;
	while( current != NULL ) {
		if( current == frame )
			return TRUE;
		if( current == owner )
			break;
		current = current->previous;
	}
	return FALSE;
}

int fb_ErrorFrameDispatch( FB_ERRORCTX *error, FB_ERRHANDLER *target, int *fatal_error )
{
	FB_ERRORFRAMECTX *context = hGetContext();
	FB_ERROR_CALLBACK *handler;
	FB_ERROR_DISPATCH active;
	FB_ERROR_DISPATCH *previous;
	FB_ERROR_OWNER *owner;
	void *label;
	int action;

	if( context == NULL )
		return 0;
	*fatal_error = error->err_num;
	if( context->unwinding != NULL )
		return -1;
	handler = hFindHandler( context, error->handler );
	if( handler == NULL )
		return 0;
	for( previous = context->dispatch; previous != NULL; previous = previous->previous ) {
		if( previous->handler == handler )
			return -1;
	}
	owner = handler->owner;
	active.previous = context->dispatch;
	active.handler = handler;
	active.anchor = context->top;
	active.error = *error;
	context->dispatch = &active;
	action = handler->callback( owner->context );
	context->dispatch = active.previous;
	if( context->top != active.anchor ) {
		*fatal_error = FB_RTERROR_ILLEGALFUNCTIONCALL;
		return -1;
	}
	if( action == FB_ERROR_FRAME_TRANSFER ) {
		if( (owner->flags & FB_ERROR_OWNER_READY) == 0 || owner->target <= 0 ) {
			*fatal_error = FB_RTERROR_ILLEGALRESUME;
			return -1;
		}
		/* Discard dispatch records whose C activations the jump abandons.
		 * An outer callback below a nested owner checkpoint remains alive. */
		while( context->dispatch != NULL &&
		       hFrameAtOrAbove( context, context->dispatch->anchor, owner->frame ) )
			context->dispatch = context->dispatch->previous;
		while( context->top != owner->frame ) {
			FB_ERROR_FRAME *frame = context->top;
			FB_ERROR_FRAME_CLEANUP cleanup = frame->cleanup;
			void *data = frame->context;
			/* Cleanup may release the allocation containing this frame. Pop
			 * its metadata first, while the C activation is still alive. */
			hPopFrame( context, error );
			context->unwinding = context->top;
			if( cleanup != NULL )
				cleanup( data );
			if( context->top != context->unwinding ) {
				context->unwinding = NULL;
				*fatal_error = FB_RTERROR_ILLEGALFUNCTIONCALL;
				return -1;
			}
			context->unwinding = NULL;
		}
		/* An ordinary handler GOTO retains ERR but cannot retain a resume
		 * address in an activation that has just been abandoned. */
		error->res_lbl = NULL;
		error->resnxt_lbl = NULL;
		fb_ErrorFrameRestoreState( error );
#if defined( _WIN32 ) && defined( __aarch64__ )
		__mingw_longjmp( owner->buffer, 1 );
#else
		longjmp( owner->buffer, 1 );
#endif
	}
	if( action == FB_ERROR_FRAME_RETRY )
		label = active.error.res_lbl;
	else if( action == FB_ERROR_FRAME_NEXT )
		label = active.error.resnxt_lbl;
	else
		return -1;
	if( label == NULL ) {
		*fatal_error = FB_RTERROR_ILLEGALRESUME;
		return -1;
	}
	error->res_lbl = NULL;
	error->resnxt_lbl = NULL;
	error->err_num = FB_RTERROR_OK;
	*target = hLegacyAddress( label );
	fb_ErrorFrameRestoreState( error );
	return 1;
}

/* end of errors/error_frame.c */
