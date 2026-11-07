/*
 * Project: FreeBASIC runtime library
 * File: drivers/win32/x86_64/error_frame.c
 * Purpose: Dispatch C error callbacks without jumping between C functions.
 * Responsibilities: Own callback metadata, preserve nested error state, and
 *     clean up live child activations before a one-way owner transfer.
 * This file intentionally does NOT contain: BASIC outlining or caller storage.
 */
#include "fb.h"
#include "error_c_resume_internal.h"
#include <setjmp.h>

#if defined( _WIN32 ) && defined( __aarch64__ )
void __mingw_longjmp( jmp_buf buffer, int value );
#endif

/* ------------------------------------------------------------------------- */
/* Activation ownership and callback metadata                                 */
/* ------------------------------------------------------------------------- */

typedef struct FB_ERROR_OWNER FB_ERROR_OWNER;
typedef struct FB_ERROR_CALLBACK FB_ERROR_CALLBACK;
typedef struct FB_ERROR_C_RESUME FB_ERROR_C_RESUME;
typedef struct FB_ERROR_NATIVE_HANDLER FB_ERROR_NATIVE_HANDLER;

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
	FB_ERROR_C_RESUME *resume_boundary;
	int target;
	unsigned flags;
};

typedef struct FB_ERROR_DISPATCH {
	struct FB_ERROR_DISPATCH *previous;
	FB_ERROR_CALLBACK *handler;
	FB_ERROR_NATIVE_HANDLER *native;
	struct FB_ERROR_DISPATCH *native_previous;
	FB_ERROR_FRAME *anchor;
	FB_ERRORCTX error;
} FB_ERROR_DISPATCH;

/* These records belong to live C activations. Their jump buffers live
 * in owned storage so a legacy handler cannot overwrite them on its stack.
 * Frame retirement removes anchored records; owner checkpoints retain the
 * older resume boundary across a one-way transfer. */
struct FB_ERROR_C_RESUME {
    jmp_buf buffer;
    struct FB_ERROR_C_RESUME *previous;
    void *retry;
    void *next;
    void *selected;
    FB_ERROR_FRAME *anchor;
};

/* A label address alone cannot distinguish recursive native activations.
 * Saved handlers therefore carry a registered opaque identity, as C callbacks
 * already do. Each native owner retains the frame captured by the leaf wrapper.
 * The explicit leave hook retires its handles before that frame expires. */
typedef struct FB_ERROR_NATIVE_OWNER FB_ERROR_NATIVE_OWNER;
struct FB_ERROR_NATIVE_HANDLER {
	struct FB_ERROR_NATIVE_HANDLER *next;
	FB_ERROR_NATIVE_OWNER *owner;
	FB_ERRHANDLER target;
};

struct FB_ERROR_NATIVE_OWNER {
	FB_ERROR_NATIVE_OWNER *next;
	FB_ERROR_NATIVE_HANDLER *handlers;
	void *frame;
	FB_ERRHANDLER previous_handler;
	size_t outgoing_space;
	FB_ERROR_FRAME *c_boundary;
	FB_ERROR_C_RESUME *resume_boundary;
	FB_ERROR_DISPATCH *dispatch_boundary;
	size_t boundary_state;
};

typedef struct FB_ERRORFRAMECTX {
	FB_ERROR_FRAME *top;
	FB_ERROR_OWNER *owners;
	FB_ERROR_DISPATCH *dispatch;
	FB_ERROR_DISPATCH *native_dispatch;
	FB_ERROR_FRAME *unwinding;
	FB_ERROR_C_RESUME *resume;
	FB_ERROR_NATIVE_OWNER *native_owners;
} FB_ERRORFRAMECTX;

enum {
	FB_ERROR_OWNER_READY = 1,
	FB_ERROR_OWNER_ACTIVATED = 2
};

enum {
	FB_ERROR_NATIVE_BOUNDARIES_VALID = 1,
	FB_ERROR_NATIVE_ABANDONED_CHILDREN = 2
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

/* Project: FreeBASIC private native ERROR activation bridge
 * File: native-owner-helpers.c.inc
 * Purpose: Find and retire opaque native-handler identities.
 * Responsibilities: Compare registered values without dereferencing caller data.
 * This file intentionally does NOT transfer control or inspect native frames.
 */
static FB_ERROR_NATIVE_HANDLER *hFindNativeHandler( FB_ERRORFRAMECTX *context, FB_ERRHANDLER token )
{
	FB_ERROR_NATIVE_OWNER *owner;
	void *address = hDataAddress( token );
	for( owner = context->native_owners; owner != NULL; owner = owner->next ) {
		FB_ERROR_NATIVE_HANDLER *handler;
		for( handler = owner->handlers; handler != NULL; handler = handler->next ) {
			if( handler == address )
				return handler;
		}
	}
	return NULL;
}

static void hFreeNativeOwner( FB_ERROR_NATIVE_OWNER *owner )
{
	FB_ERROR_NATIVE_HANDLER *handler = owner->handlers;
	while( handler != NULL ) {
		FB_ERROR_NATIVE_HANDLER *next = handler->next;
		free( handler );
		handler = next;
	}
	free( owner );
}

static int hNativeOwnerReferenced( FB_ERRORFRAMECTX *context, FB_ERROR_NATIVE_OWNER *native )
{
	FB_ERROR_OWNER *owner;
	FB_ERROR_DISPATCH *dispatch;
	FB_ERROR_NATIVE_HANDLER *handler;
	for( owner = context->owners; owner != NULL; owner = owner->next ) {
		handler = hFindNativeHandler( context, owner->previous_handler );
		if( handler != NULL && handler->owner == native )
			return TRUE;
	}
	for( dispatch = context->dispatch; dispatch != NULL; dispatch = dispatch->previous ) {
		if( dispatch->native != NULL && dispatch->native->owner == native )
			return TRUE;
		/* RestoreState preserves the selected handler. The saved error
		 * snapshot therefore does not retain its old handler slot. */
	}
	return FALSE;
}
/* end of native-owner-helpers.c.inc */

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
	/* Only native records are owned allocations. Their separate chain stays
	 * valid even when TLS teardown can no longer inspect borrowed C stacks. */
	while( context->native_dispatch != NULL ) {
		FB_ERROR_DISPATCH *pending = context->native_dispatch;
		context->native_dispatch = pending->native_previous;
		free( pending );
	}
	context->unwinding = NULL;
	while( context->native_owners != NULL ) {
		FB_ERROR_NATIVE_OWNER *native = context->native_owners;
		context->native_owners = native->next;
		hFreeNativeOwner( native );
	}
    while( context->resume != NULL ) {
        FB_ERROR_C_RESUME *node = context->resume;
        context->resume = node->previous;
        free( node );
    }
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
	((FB_ERROR_OWNER *)frame->owner)->resume_boundary = context->resume;
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
	return hDataAddress( fb_ErrorSetHandlerRaw( hLegacyAddress( handler ) ) );
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

/* A caller can explicitly pop a record while an owner still refers to its
 * boundary. Invalidate that checkpoint before freeing the record so a later
 * allocation at the same address cannot revive the old boundary identity. */
static void hFreeResume( FB_ERRORFRAMECTX *context, FB_ERROR_C_RESUME *node )
{
	FB_ERROR_OWNER *owner;
	for( owner = context->owners; owner != NULL; owner = owner->next ) {
		if( owner->resume_boundary == node ) {
			owner->resume_boundary = NULL;
			owner->flags &= ~(unsigned)FB_ERROR_OWNER_READY;
		}
	}
	/* Native owner entry boundaries obey the same no-reuse rule as C owner
	 * checkpoints. An explicitly popped boundary cannot become live again. */
	for( FB_ERROR_NATIVE_OWNER *native = context->native_owners; native != NULL; native = native->next ) {
		if( native->resume_boundary == node ) {
			native->resume_boundary = NULL;
			native->boundary_state &= ~(size_t)FB_ERROR_NATIVE_BOUNDARIES_VALID;
		}
	}
	free( node );
}

/* A frame may be retired before its failure checkpoint returns. Unlink its
 * records while the borrowed frame identity is still valid; cleanup can free
 * the allocation containing the frame immediately after this function. */
static void hDiscardFrameResumes( FB_ERRORFRAMECTX *context, FB_ERROR_FRAME *frame, FB_ERRORCTX *error )
{
	FB_ERROR_C_RESUME **link = &context->resume;
	while( *link != NULL ) {
		FB_ERROR_C_RESUME *node = *link;
		if( node->anchor != frame ) {
			link = &node->previous;
			continue;
		}
		if( error != NULL ) {
			if( error->res_lbl == node->retry || error->res_lbl == node->next )
				error->res_lbl = NULL;
			if( error->resnxt_lbl == node->retry || error->resnxt_lbl == node->next )
				error->resnxt_lbl = NULL;
		}
		*link = node->previous;
		hFreeResume( context, node );
	}
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
	hDiscardFrameResumes( context, frame, error );
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
		if( dispatch->anchor == storage || (dispatch->handler != NULL && dispatch->handler->owner->frame == storage) )
			return FB_RTERROR_ILLEGALFUNCTIONCALL;
	}
	error = (FB_ERRORCTX *)fb_TlsGetCtxExisting( FB_TLSKEY_ERROR );
	hPopFrame( context, error );
	return FB_RTERROR_OK;
}

/* ------------------------------------------------------------------------- */
/* Nested error state and one-way owner transfers                             */
/* ------------------------------------------------------------------------- */

/* Cleanup is a transition into arbitrary caller code. Keep its temporary
 * error context in one activation; outlining also prevents GCC from expanding
 * those context copies through the frame-list traversal. */
static void __attribute__((noinline)) hRunCleanup
	( FB_ERRORCTX *error, FB_ERROR_FRAME_CLEANUP cleanup, void *data )
{
	FB_ERRORCTX saved_error = *error;
	cleanup( data );
	*error = saved_error;
}

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

/* C callbacks borrow a live stack record; native handlers need owned storage
 * because the handoff never returns to its C call site. Both kinds participate
 * in one dispatch order, including alternating C/native nesting. */
static void hDropDispatch( FB_ERRORFRAMECTX *context )
{
	FB_ERROR_DISPATCH *active = context->dispatch;
	if( active->native != NULL ) {
		if( context->native_dispatch != active )
			fb_End( FB_RTERROR_ILLEGALRESUME );
		context->native_dispatch = active->native_previous;
	}
	context->dispatch = active->previous;
	if( active->native != NULL )
		free( active );
}

/* A source RESUME consumes the innermost native activation. General state
 * restoration also runs after C callbacks, so it must not consume a surviving
 * outer native handler merely because that handler is now at the chain head. */
void fb_ErrorNativeResumeState( void *label )
{
	FB_ERRORFRAMECTX *context = hGetContext();
	FB_ERROR_DISPATCH *active = context != NULL ? context->dispatch : NULL;
	if( active == NULL || active->native == NULL )
		return;
	if( label != active->error.res_lbl && label != active->error.resnxt_lbl )
		fb_End( FB_RTERROR_ILLEGALRESUME );
	hDropDispatch( context );
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

/* A callback can allocate a checkpoint anchored in the surviving owner's
 * frame. Frame removal alone cannot retire it. The owner's ready point marks
 * the older records whose C activations the owner jump must preserve. */
static int hResumeBoundaryExists( FB_ERRORFRAMECTX *context, FB_ERROR_C_RESUME *boundary )
{
	FB_ERROR_C_RESUME *node;
	if( boundary == NULL )
		return TRUE;
	for( node = context->resume; node != NULL; node = node->previous ) {
		if( node == boundary )
			return TRUE;
	}
	return FALSE;
}

static void hDiscardNewerResumes( FB_ERRORFRAMECTX *context, FB_ERROR_C_RESUME *boundary )
{
	while( context->resume != boundary ) {
		FB_ERROR_C_RESUME *node = context->resume;
		context->resume = node->previous;
		hFreeResume( context, node );
	}
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
	active.native = NULL;
	active.native_previous = NULL;
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
		if( (owner->flags & FB_ERROR_OWNER_READY) == 0 || owner->target <= 0 ||
		    hResumeBoundaryExists( context, owner->resume_boundary ) == FALSE ) {
			*fatal_error = FB_RTERROR_ILLEGALRESUME;
			return -1;
		}
		/* Discard dispatch records whose C activations the jump abandons.
		 * An outer callback below a nested owner checkpoint remains alive. */
		while( context->dispatch != NULL &&
		       hFrameAtOrAbove( context, context->dispatch->anchor, owner->frame ) )
			hDropDispatch( context );
		hDiscardNewerResumes( context, owner->resume_boundary );
		while( context->top != owner->frame ) {
			FB_ERROR_FRAME *frame = context->top;
			FB_ERROR_FRAME_CLEANUP cleanup = frame->cleanup;
			void *data = frame->context;
			/* Cleanup may release the allocation containing this frame. Pop
			 * its metadata first, while the C activation is still alive. */
			hPopFrame( context, error );
			context->unwinding = context->top;
			if( cleanup != NULL )
				hRunCleanup( error, cleanup, data );
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

/* ------------------------------------------------------------------------- */
/* Private resumable C activation experiment                                  */
/* ------------------------------------------------------------------------- */

FBCALL void *fb_ErrorCResumePush( void *retry, void *next )
{
    FB_ERRORFRAMECTX *context;
    FB_ERROR_C_RESUME *node;
    if( retry == NULL || next == NULL || retry == next ) {
        fb_ErrorSetNum( FB_RTERROR_ILLEGALRESUME );
        return NULL;
    }
    context = FB_TLSGETCTX( ERRORFRAME );
    if( context == NULL ) {
        fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
        return NULL;
    }
    node = (FB_ERROR_C_RESUME *)calloc( 1, sizeof( *node ) );
    if( node == NULL ) {
        fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
        return NULL;
    }
    node->previous = context->resume;
    node->retry = retry;
    node->next = next;
    context->resume = node;
    return &node->buffer;
}

/* Tokens are object identities, not writable memory. C guarantees the
 * same representation for qualified/unqualified object pointers; copy that
 * representation rather than removing qualification through a cast. */
FBCALL void *fb_ErrorCResumeEnter( void *frame, const void *retry, const void *next )
{
	FB_ERRORFRAMECTX *context = hGetContext();
	void *mutable_retry, *mutable_next, *buffer;
	if( frame != NULL && (context == NULL || hFindFrame( context, frame ) == NULL) ) {
		fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
		return NULL;
	}
	memcpy( &mutable_retry, &retry, sizeof( mutable_retry ) );
	memcpy( &mutable_next, &next, sizeof( mutable_next ) );
	buffer = fb_ErrorCResumePush( mutable_retry, mutable_next );
	if( buffer != NULL ) {
		context = hGetContext();
		context->resume->anchor = frame;
	}
	return buffer;
}

FBCALL void *fb_ErrorCResumeTarget( void *buffer )
{
    FB_ERRORFRAMECTX *context = hGetContext();
    FB_ERROR_C_RESUME *node;
    if( context == NULL )
        return NULL;
    for( node = context->resume; node != NULL; node = node->previous ) {
        if( (void *)&node->buffer == buffer )
            return node->selected;
    }
    return NULL;
}

FBCALL int fb_ErrorCResumePop( void *buffer )
{
    FB_ERRORFRAMECTX *context = hGetContext();
    FB_ERROR_C_RESUME *node;
    if( context == NULL || context->resume == NULL || (void *)&context->resume->buffer != buffer )
        return fb_ErrorSetNum( FB_RTERROR_ILLEGALRESUME );
    node = context->resume;
    context->resume = node->previous;
    hFreeResume( context, node );
    return FB_RTERROR_OK;
}

int fb_ErrorCResumeToken( void *token )
{
    FB_ERRORFRAMECTX *context = hGetContext();
    FB_ERROR_C_RESUME *node;
    if( context == NULL )
        return FALSE;
    for( node = context->resume; node != NULL; node = node->previous ) {
        if( node->retry != token && node->next != token )
            continue;
        node->selected = token;
#if defined( _WIN32 ) && defined( __aarch64__ )
        __mingw_longjmp( node->buffer, 1 );
#else
        longjmp( node->buffer, 1 );
#endif
    }
    return FALSE;
}

/* Project: FreeBASIC private native ERROR activation bridge
 * File: native-owner-api.c.inc
 * Purpose: Select and restore native handlers by activation identity.
 * Responsibilities: Preserve executable targets, owner frames and bounded lifetimes.
 * This file intentionally does NOT infer retirement from a reused stack address.
 */
FBCALL FB_ERRHANDLER fb_ErrorSetNativeHandlerSized( FB_ERRHANDLER handler, void *frame, size_t outgoing_space )
{
	FB_ERRORFRAMECTX *context = hGetContext();
	FB_ERROR_NATIVE_OWNER *owner;
	FB_ERROR_NATIVE_HANDLER *native;
	int new_owner = FALSE;

	/* fb_End terminates the process. Its shared C declaration does not
	 * expose that contract, so keep refusal paths explicit here. */

	if( context != NULL && context->unwinding != NULL ) {
		fb_End( FB_RTERROR_ILLEGALFUNCTIONCALL );
		return NULL;
	}
	/* Saved compiler values are opaque registered identities. Restoring one
	 * must retain its original owner, regardless of this caller's frame base. */
	if( handler == NULL || (context != NULL &&
	    (hFindNativeHandler( context, handler ) != NULL || hFindHandler( context, handler ) != NULL)) )
		return fb_ErrorSetHandlerRaw( handler );
	if( frame == NULL || outgoing_space < 32 || outgoing_space > (size_t)PTRDIFF_MAX - 15 || outgoing_space % 8 != 0 ) {
		fb_End( FB_RTERROR_ILLEGALFUNCTIONCALL );
		return NULL;
	}
	if( context == NULL )
		context = FB_TLSGETCTX( ERRORFRAME );
	if( context == NULL ) {
		fb_End( FB_RTERROR_OUTOFMEM );
		return NULL;
	}
	for( owner = context->native_owners; owner != NULL; owner = owner->next ) {
		if( owner->frame == frame )
			break;
	}
	if( owner == NULL ) {
		owner = (FB_ERROR_NATIVE_OWNER *)calloc( 1, sizeof( *owner ) );
		if( owner == NULL ) {
			fb_End( FB_RTERROR_OUTOFMEM );
			return NULL;
		}
		owner->frame = frame;
		owner->outgoing_space = outgoing_space;
		owner->c_boundary = context->top;
		owner->resume_boundary = context->resume;
		owner->dispatch_boundary = context->dispatch;
		owner->boundary_state = FB_ERROR_NATIVE_BOUNDARIES_VALID;
		owner->previous_handler = FB_TLSGETCTX( ERROR )->handler;
		new_owner = TRUE;
	}
	for( native = owner->handlers; native != NULL; native = native->next ) {
		if( native->target == handler )
			break;
	}
	if( native == NULL ) {
		native = (FB_ERROR_NATIVE_HANDLER *)calloc( 1, sizeof( *native ) );
		if( native == NULL ) {
			if( new_owner )
				free( owner );
			fb_End( FB_RTERROR_OUTOFMEM );
			return NULL;
		}
		native->owner = owner;
		native->target = handler;
		native->next = owner->handlers;
		owner->handlers = native;
	}
	if( new_owner ) {
		owner->next = context->native_owners;
		context->native_owners = owner;
	}
	return fb_ErrorSetHandlerRaw( hLegacyAddress( native ) );
}

/* Direct private API users retain the original four-register argument area.
 * The public leaf wrapper supplies a conservative complete-frame bound for
 * older GAS objects; new GAS thunks supply their exact outgoing argument size. */
FBCALL FB_ERRHANDLER fb_ErrorSetNativeHandler( FB_ERRHANDLER handler, void *frame )
{
	return fb_ErrorSetNativeHandlerSized( handler, frame, 32 );
}

FBCALL FB_ERRHANDLER fb_ErrorSetNativeHandlerLegacy( FB_ERRHANDLER handler, void *frame, void *stack )
{
	uintptr_t base = (uintptr_t)frame;
	uintptr_t bottom = (uintptr_t)stack;
	size_t bound = base >= bottom ? (size_t)(base - bottom) : 0;
	return fb_ErrorSetNativeHandlerSized( handler, frame, bound );
}

FBCALL void fb_ErrorNativeDispatch( void *target )
{
	FB_ERRORFRAMECTX *context = hGetContext();
	FB_ERRORCTX *error = (FB_ERRORCTX *)fb_TlsGetCtxExisting( FB_TLSKEY_ERROR );
	FB_ERROR_NATIVE_HANDLER *native = context != NULL && error != NULL ? hFindNativeHandler( context, error->handler ) : NULL;
	FB_ERROR_DISPATCH *active;
	if( native == NULL || hDataAddress( native->target ) != target ) {
		fb_End( FB_RTERROR_ILLEGALRESUME );
		__builtin_unreachable();
	}
	/* Preserve this error while the native handler can call another failing
	 * C worker. SaveState updates the active record before a nested throw. */
	active = (FB_ERROR_DISPATCH *)calloc( 1, sizeof( *active ) );
	if( active == NULL ) {
		fb_End( FB_RTERROR_OUTOFMEM );
		__builtin_unreachable();
	}
	active->previous = context->dispatch;
	active->native = native;
	active->native_previous = context->native_dispatch;
	active->anchor = context->top;
	active->error = *error;
	context->native_dispatch = active;
	context->dispatch = active;
	fb_ErrorNativeDispatchRaw( target, native->owner->frame, native->owner->outgoing_space );
}

FBCALL FB_ERRHANDLER fb_ErrorNativeHandlerTarget( FB_ERRHANDLER handler )
{
	FB_ERRORFRAMECTX *context = hGetContext();
	FB_ERROR_NATIVE_HANDLER *native = context != NULL ? hFindNativeHandler( context, handler ) : NULL;
	return native != NULL ? native->target : handler;
}

FBCALL void *fb_ErrorNativeHandlerFrame( void *handler )
{
	FB_ERRORFRAMECTX *context = hGetContext();
	FB_ERRORCTX *error = (FB_ERRORCTX *)fb_TlsGetCtxExisting( FB_TLSKEY_ERROR );
	FB_ERROR_NATIVE_HANDLER *native;
	if( context == NULL || error == NULL || handler == NULL )
		return NULL;
	native = hFindNativeHandler( context, error->handler );
	if( native == NULL || hDataAddress( native->target ) != handler )
		return NULL;
	return native->owner->frame;
}

/* Project: FreeBASIC native owner return cleanup
 * File: native-return.c.inc
 * Purpose: Retire C children before a native owner starts its local cleanup.
 * Responsibilities: Validate entry boundaries and preserve older activations.
 * This file intentionally does NOT discard newer native owners or restore RSP.
 */
static int hDispatchBoundaryExists( FB_ERRORFRAMECTX *context, FB_ERROR_DISPATCH *boundary )
{
	FB_ERROR_DISPATCH *active;
	if( boundary == NULL )
		return TRUE;
	for( active = context->dispatch; active != NULL; active = active->previous ) {
		if( active == boundary )
			return TRUE;
	}
	return FALSE;
}

FBCALL int fb_ErrorNativeOwnerCleanup( void *storage )
{
	FB_ERRORFRAMECTX *context = hGetContext();
	FB_ERROR_NATIVE_OWNER *owner;
	FB_ERRORCTX *error;
	if( storage == NULL )
		return FB_RTERROR_ILLEGALFUNCTIONCALL;
	if( context == NULL )
		return FB_RTERROR_OK;
	for( owner = context->native_owners; owner != NULL; owner = owner->next ) {
		if( owner->frame == storage )
			break;
	}
	/* Conditional registration may not have executed on this source exit. */
	if( owner == NULL )
		return FB_RTERROR_OK;
	if( context->unwinding != NULL || context->native_owners != owner )
		return FB_RTERROR_ILLEGALFUNCTIONCALL;
	if( context->top == owner->c_boundary && context->resume == owner->resume_boundary &&
	    context->dispatch == owner->dispatch_boundary )
		return FB_RTERROR_OK;
	if( (owner->boundary_state & FB_ERROR_NATIVE_BOUNDARIES_VALID) == 0 ||
	    (owner->c_boundary != NULL && hFindFrame( context, owner->c_boundary ) == NULL) ||
	    hResumeBoundaryExists( context, owner->resume_boundary ) == FALSE ||
	    hDispatchBoundaryExists( context, owner->dispatch_boundary ) == FALSE )
		return FB_RTERROR_ILLEGALRESUME;
	error = (FB_ERRORCTX *)fb_TlsGetCtxExisting( FB_TLSKEY_ERROR );
	if( error == NULL )
		return FB_RTERROR_ILLEGALFUNCTIONCALL;
	while( context->dispatch != owner->dispatch_boundary )
		hDropDispatch( context );
	hDiscardNewerResumes( context, owner->resume_boundary );
	while( context->top != owner->c_boundary ) {
		FB_ERROR_FRAME *frame = context->top;
		FB_ERROR_FRAME_CLEANUP cleanup = frame->cleanup;
		void *data = frame->context;
		FB_ERROR_FRAME *surviving_top;
		hPopFrame( context, error );
		surviving_top = context->top;
		/* The popped frame is a non-NULL identity even at the root. Cleanup
		 * can release its containing capture, so never dereference it again. */
		context->unwinding = frame;
		if( cleanup != NULL )
			hRunCleanup( error, cleanup, data );
		if( context->top != surviving_top || context->native_owners != owner ) {
			context->unwinding = NULL;
			return FB_RTERROR_ILLEGALFUNCTIONCALL;
		}
		context->unwinding = NULL;
	}
	/* ERR remains the handled error; its abandoned worker cannot be resumed.
	 * Preserve the native handler until its own local destructors finish. */
	error->res_lbl = NULL;
	error->resnxt_lbl = NULL;
	owner->boundary_state |= FB_ERROR_NATIVE_ABANDONED_CHILDREN;
	return FB_RTERROR_OK;
}

FBCALL void fb_ErrorNativeOwnerCleanupChecked( void *storage )
{
	int status = fb_ErrorNativeOwnerCleanup( storage );
	if( status != FB_RTERROR_OK )
		fb_End( status );
}

/* end of native-return.c.inc */

FBCALL int fb_ErrorNativeOwnerLeave( void *frame )
{
	FB_ERRORFRAMECTX *context = hGetContext();
	FB_ERROR_NATIVE_OWNER *owner;
	FB_ERROR_NATIVE_HANDLER *selected;
	FB_ERRORCTX *error;
	if( frame == NULL )
		return FB_RTERROR_ILLEGALFUNCTIONCALL;
	if( context == NULL )
		return FB_RTERROR_OK;
	for( owner = context->native_owners; owner != NULL; owner = owner->next ) {
		if( owner->frame == frame )
			break;
	}
	/* The registration statement can be bypassed. Its unconditional compiler
	 * epilogue still retires this activation, with no metadata to release. */
	if( owner == NULL )
		return FB_RTERROR_OK;
	if( context->unwinding != NULL || context->native_owners != owner )
		return FB_RTERROR_ILLEGALFUNCTIONCALL;
	if( hNativeOwnerReferenced( context, owner ) )
		return FB_RTERROR_ILLEGALFUNCTIONCALL;
	error = (FB_ERRORCTX *)fb_TlsGetCtxExisting( FB_TLSKEY_ERROR );
	selected = error != NULL ? hFindNativeHandler( context, error->handler ) : NULL;
	if( selected != NULL && selected->owner == owner )
		error->handler = owner->previous_handler;
	context->native_owners = owner->next;
	hFreeNativeOwner( owner );
	return FB_RTERROR_OK;
}
/* end of native-owner-api.c.inc */


/* The emitted epilogue cannot recover by leaving a live owner behind. */
FBCALL int fb_ErrorNativeOwnerLeaveChecked( void *frame )
{
	FB_ERRORFRAMECTX *context = hGetContext();
	FB_ERROR_NATIVE_OWNER *owner;
	int abandoned = FALSE;
	int status = fb_ErrorNativeOwnerCleanup( frame );
	if( status != FB_RTERROR_OK )
		fb_End( status );
	if( context != NULL ) {
		for( owner = context->native_owners; owner != NULL; owner = owner->next ) {
			if( owner->frame == frame ) {
				abandoned = (owner->boundary_state & FB_ERROR_NATIVE_ABANDONED_CHILDREN) != 0;
				break;
			}
		}
	}
	status = fb_ErrorNativeOwnerLeave( frame );
	if( status != FB_RTERROR_OK )
		fb_End( status );
	if( abandoned ) {
		FB_ERRORCTX *error = (FB_ERRORCTX *)fb_TlsGetCtxExisting( FB_TLSKEY_ERROR );
		if( error != NULL )
			fb_ErrorFrameRestoreState( error );
	}
	return status;
}

/* end of drivers/win32/x86_64/error_frame.c */
