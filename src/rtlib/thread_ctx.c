/* thread local context storage */

#include "fb.h"
#include "fb_private_thread.h"

#if defined ENABLE_MT && defined HOST_UNIX
	#define FB_TLSENTRY           pthread_key_t
	#define FB_TLSALLOC(key)      pthread_key_create( &(key), NULL )
	#define FB_TLSSET(key,value)  pthread_setspecific( (key), (const void *)(value) )
	#define FB_TLSGET(key)        pthread_getspecific( (key) )
	#ifdef HOST_NETBSD
		/*
		   NetBSD libpthread asserts if pthread_setspecific() is used after
		   pthread_key_delete() has released a key.  FreeBASIC can still see
		   late TLS access during process teardown, for example from global
		   destructors or threads finishing as the runtime exits.  Keep the
		   small process-lifetime key table valid and free the actual TLS
		   contexts through fb_TlsFreeCtxTb().
		*/
		#define FB_TLSFREE(key)   (void)(key)
	#else
		#define FB_TLSFREE(key)   pthread_key_delete( (key) )
	#endif
#elif defined ENABLE_MT && ( defined HOST_WIN32 || defined HOST_XBOX )
	#define FB_TLSENTRY           DWORD
	#define FB_TLSALLOC(key)      key = TlsAlloc( )
	#define FB_TLSFREE(key)       TlsFree( (key) )
	#define FB_TLSSET(key,value)  TlsSetValue( (key), (LPVOID)(value) )
	#define FB_TLSGET(key)        TlsGetValue( (key) )
#else
	#define FB_TLSENTRY           uintptr_t
	#define FB_TLSALLOC(key)      key = NULL
	#define FB_TLSFREE(key)       key = NULL
	#define FB_TLSSET(key,value)  key = (FB_TLSENTRY)value
	#define FB_TLSGET(key)        key
#endif

static FB_TLSENTRY __fb_tls_ctxtb[FB_TLSKEYS];

#define FB_TLS_DATA_TO_HEADER( data ) ( ( ( FB_TLS_CTX_HEADER *)data ) - 1 )
#define FB_TLS_HEADER_TO_DATA( header ) ( ( void *) ( header + 1 ) )

/* Retrieve or create new TLS context for given key */
FBCALL void *fb_TlsGetCtx( int index, size_t len, FB_TLS_DESTRUCTOR destructorFn )
{
	void *ctx = (void *)FB_TLSGET( __fb_tls_ctxtb[index] );

	if( ctx == NULL ) {
		FB_TLS_CTX_HEADER *ctxHeader = (FB_TLS_CTX_HEADER *)calloc( 1, len + sizeof(FB_TLS_CTX_HEADER) );
		if( ctxHeader != NULL ) {
			ctxHeader->destructor = destructorFn;
			ctx = FB_TLS_HEADER_TO_DATA( ctxHeader );

			#if defined ENABLE_MT && defined HOST_UNIX
				if( FB_TLSSET( __fb_tls_ctxtb[index], ctx ) != 0 ) {
					free( ctxHeader );
					ctx = NULL;
				}
			#elif defined ENABLE_MT && ( defined HOST_WIN32 || defined HOST_XBOX )
				if( FB_TLSSET( __fb_tls_ctxtb[index], ctx ) == 0 ) {
					free( ctxHeader );
					ctx = NULL;
				}
			#else
				FB_TLSSET( __fb_tls_ctxtb[index], ctx );
			#endif
		}

		/*
		   ctxHeader is now either freed or owned by platform TLS.  cppcheck
		   does not model the ownership transfer through pthread_setspecific().
		*/
		/* cppcheck-suppress memleak */
	}
#ifdef DEBUG
	else {
		FB_TLS_CTX_HEADER *ctxHeader = FB_TLS_DATA_TO_HEADER( ctx );
		/* The && is intentional here so if the assert fires, the message is displayed too */
		DBG_ASSERT( (ctxHeader->destructor == destructorFn) && "fb_TlsGetCtx trying to set different destructor for existing data" );
	}
#endif

	return ctx;
}

FBCALL void fb_TlsDelCtx( int index )
{
	void *ctx = (void *)FB_TLSGET( __fb_tls_ctxtb[index] );

	/* free mem block if any */
	if( ctx != NULL ) {
		FB_TLS_CTX_HEADER *ctxHeader = FB_TLS_DATA_TO_HEADER( ctx );
		if( ctxHeader->destructor ) {
			( ctxHeader->destructor )( ctx );
		}
		free( ctxHeader );
		FB_TLSSET( __fb_tls_ctxtb[index], NULL );
	}
}

FBCALL void fb_TlsFreeCtxTb( void )
{
	/* free all thread local storage ctx's */
	int i;
	for( i = 0; i < FB_TLSKEYS; i++ ) {
		fb_TlsDelCtx( i );
	}
}

#ifdef ENABLE_MT
void fb_TlsInit( void )
{
	/* allocate thread local storage keys */
	int i;
	for( i = 0; i < FB_TLSKEYS; i++ )
		FB_TLSALLOC( __fb_tls_ctxtb[i] );
}

void fb_TlsExit( void )
{
	/* free thread local storage keys */
	int i;
	for( i = 0; i < FB_TLSKEYS; i++ ) {
		FB_TLSFREE( __fb_tls_ctxtb[i] );
	}
	fb_CloseAtomicFBThreadFlagMutex( );
}
#endif
