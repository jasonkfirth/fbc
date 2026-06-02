/*
    FreeBASIC runtime Wii thread local storage
    ------------------------------------------

    File: thread_ctx.c

    Purpose:

        Adapt FreeBASIC's runtime TLS hooks to libogc LWP threads.

    Responsibilities:

        - allocate per-thread FreeBASIC TLS contexts
        - run TLS destructors when thread contexts are released
        - keep the single-threaded fallback available for non-MT builds

    This file intentionally does NOT contain:

        - thread creation
        - mutex implementation
        - scheduler policy
*/

#include "../fb.h"
#include "../fb_private_thread.h"

#define FB_TLS_DATA_TO_HEADER( data ) ( ( ( FB_TLS_CTX_HEADER *)data ) - 1 )
#define FB_TLS_HEADER_TO_DATA( header ) ( ( void *) ( header + 1 ) )

static void hWiiFreeCtx( void *ctx )
{
	if( ctx != NULL ) {
		FB_TLS_CTX_HEADER *ctxHeader = FB_TLS_DATA_TO_HEADER( ctx );
		if( ctxHeader->destructor ) {
			( ctxHeader->destructor )( ctx );
		}
		free( ctxHeader );
	}
}

#ifdef ENABLE_MT

typedef struct _FB_WII_TLSNODE
{
	lwp_t id;
	void *ctx[FB_TLSKEYS];
	struct _FB_WII_TLSNODE *next;
} FB_WII_TLSNODE;

static mutex_t __fb_wii_tls_mutex = LWP_MUTEX_NULL;
static FB_WII_TLSNODE *__fb_wii_tls_nodes = NULL;
static int __fb_wii_tls_ready = FALSE;

static FB_WII_TLSNODE *hWiiFindTlsNode( lwp_t id, int create )
{
	FB_WII_TLSNODE *node = __fb_wii_tls_nodes;

	while( node != NULL ) {
		if( node->id == id )
			return node;
		node = node->next;
	}

	if( create == FALSE )
		return NULL;

	node = (FB_WII_TLSNODE *)calloc( 1, sizeof( FB_WII_TLSNODE ) );
	if( node == NULL )
		return NULL;

	node->id = id;
	node->next = __fb_wii_tls_nodes;
	__fb_wii_tls_nodes = node;

	return node;
}

/* Retrieve or create new TLS context for given key */
FBCALL void *fb_TlsGetCtx( int index, size_t len, FB_TLS_DESTRUCTOR destructorFn )
{
	FB_WII_TLSNODE *node;
	void *ctx = NULL;

	if( __fb_wii_tls_ready == FALSE )
		return NULL;

	LWP_MutexLock( __fb_wii_tls_mutex );

	node = hWiiFindTlsNode( LWP_GetSelf(), TRUE );
	if( node != NULL ) {
		ctx = node->ctx[index];
		if( ctx == NULL ) {
			FB_TLS_CTX_HEADER *ctxHeader = (FB_TLS_CTX_HEADER *)calloc( 1, len + sizeof( FB_TLS_CTX_HEADER ) );
			if( ctxHeader != NULL ) {
				ctxHeader->destructor = destructorFn;
				ctx = FB_TLS_HEADER_TO_DATA( ctxHeader );
				node->ctx[index] = ctx;
			}
		}
#ifdef DEBUG
		else {
			FB_TLS_CTX_HEADER *ctxHeader = FB_TLS_DATA_TO_HEADER( ctx );
			DBG_ASSERT( (ctxHeader->destructor == destructorFn) && "fb_TlsGetCtx trying to set different destructor for existing data" );
		}
#endif
	}

	LWP_MutexUnlock( __fb_wii_tls_mutex );

	return ctx;
}

FBCALL void fb_TlsDelCtx( int index )
{
	FB_WII_TLSNODE *node;
	void *ctx = NULL;

	if( __fb_wii_tls_ready == FALSE )
		return;

	LWP_MutexLock( __fb_wii_tls_mutex );

	node = hWiiFindTlsNode( LWP_GetSelf(), FALSE );
	if( node != NULL ) {
		ctx = node->ctx[index];
		node->ctx[index] = NULL;
	}

	LWP_MutexUnlock( __fb_wii_tls_mutex );

	hWiiFreeCtx( ctx );
}

FBCALL void fb_TlsFreeCtxTb( void )
{
	FB_WII_TLSNODE *node;
	FB_WII_TLSNODE **owner;
	void *ctx[FB_TLSKEYS];
	int i;

	if( __fb_wii_tls_ready == FALSE )
		return;

	memset( ctx, 0, sizeof( ctx ) );

	LWP_MutexLock( __fb_wii_tls_mutex );

	owner = &__fb_wii_tls_nodes;
	node = __fb_wii_tls_nodes;
	while( node != NULL ) {
		if( node->id == LWP_GetSelf() ) {
			memcpy( ctx, node->ctx, sizeof( ctx ) );
			*owner = node->next;
			free( node );
			break;
		}
		owner = &node->next;
		node = node->next;
	}

	LWP_MutexUnlock( __fb_wii_tls_mutex );

	for( i = 0; i < FB_TLSKEYS; i++ )
		hWiiFreeCtx( ctx[i] );
}

void fb_TlsInit( void )
{
	if( __fb_wii_tls_ready )
		return;

	if( LWP_MutexInit( &__fb_wii_tls_mutex, TRUE ) == 0 )
		__fb_wii_tls_ready = TRUE;
}

void fb_TlsExit( void )
{
	FB_WII_TLSNODE *node;

	if( __fb_wii_tls_ready == FALSE )
		return;

	LWP_MutexLock( __fb_wii_tls_mutex );
	node = __fb_wii_tls_nodes;
	__fb_wii_tls_nodes = NULL;
	LWP_MutexUnlock( __fb_wii_tls_mutex );

	while( node != NULL ) {
		FB_WII_TLSNODE *next = node->next;
		int i;
		for( i = 0; i < FB_TLSKEYS; i++ )
			hWiiFreeCtx( node->ctx[i] );
		free( node );
		node = next;
	}

	LWP_MutexDestroy( __fb_wii_tls_mutex );
	__fb_wii_tls_mutex = LWP_MUTEX_NULL;
	__fb_wii_tls_ready = FALSE;

	fb_CloseAtomicFBThreadFlagMutex( );
}

#else

static uintptr_t __fb_tls_ctxtb[FB_TLSKEYS];

/* Retrieve or create new TLS context for given key */
FBCALL void *fb_TlsGetCtx( int index, size_t len, FB_TLS_DESTRUCTOR destructorFn )
{
	void *ctx = (void *)__fb_tls_ctxtb[index];

	if( ctx == NULL ) {
		FB_TLS_CTX_HEADER *ctxHeader = (FB_TLS_CTX_HEADER *)calloc( 1, len + sizeof( FB_TLS_CTX_HEADER ) );
		if( ctxHeader != NULL ) {
			ctxHeader->destructor = destructorFn;
			ctx = FB_TLS_HEADER_TO_DATA( ctxHeader );
			__fb_tls_ctxtb[index] = (uintptr_t)ctx;
		}
	}
#ifdef DEBUG
	else {
		FB_TLS_CTX_HEADER *ctxHeader = FB_TLS_DATA_TO_HEADER( ctx );
		DBG_ASSERT( (ctxHeader->destructor == destructorFn) && "fb_TlsGetCtx trying to set different destructor for existing data" );
	}
#endif

	return ctx;
}

FBCALL void fb_TlsDelCtx( int index )
{
	void *ctx = (void *)__fb_tls_ctxtb[index];

	if( ctx != NULL ) {
		__fb_tls_ctxtb[index] = 0;
		hWiiFreeCtx( ctx );
	}
}

FBCALL void fb_TlsFreeCtxTb( void )
{
	int i;

	for( i = 0; i < FB_TLSKEYS; i++ )
		fb_TlsDelCtx( i );
}

#endif

/* end of thread_ctx.c */
