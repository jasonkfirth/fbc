/*
 * Project: FreeBASIC runtime tests
 * File: lpt-device-state.c
 * Purpose: Check printer-name ownership when devices are shared or fail to open.
 * Responsibilities: Exercise the production OPEN path with allocation tracking.
 * This file does not access a printer or test platform spooler behavior.
 */
#include "fb.h"

static void *allocations[16];
static int live_allocations, allocation_calls, fail_after = -1;
static int printer_result, checks;

static void check( int condition )
{
	++checks;
	if( !condition ) {
		fprintf( stderr, "LPT_DEVICE_STATE_FAIL check=%d live=%d\n", checks, live_allocations );
		exit( 1 );
	}
}

static void *tracked_calloc( size_t count, size_t size )
{
	void *p;
	size_t i;
	if( allocation_calls++ == fail_after )
		return NULL;
	p = calloc( count, size );
	if( p != NULL ) {
		for( i = 0; i < sizeof( allocations ) / sizeof( allocations[0] ); ++i ) {
			if( allocations[i] == NULL ) {
				allocations[i] = p;
				++live_allocations;
				return p;
			}
		}
		abort();
	}
	return p;
}

static void *tracked_malloc( size_t size )
{
	return tracked_calloc( 1, size );
}

static void tracked_free( void *p )
{
	size_t i;
	if( p != NULL ) {
		for( i = 0; i < sizeof( allocations ) / sizeof( allocations[0] ); ++i ) {
			if( allocations[i] == p ) {
				allocations[i] = NULL;
				--live_allocations;
				break;
			}
		}
	}
	free( p );
}

/* Track allocations owned by dev_lpt.c. The parser's separate allocation is
   intentionally outside this ledger and is released by the production path. */
#define calloc tracked_calloc
#define malloc tracked_malloc
#define free tracked_free
#ifndef FB_LPT_SOURCE
#define FB_LPT_SOURCE "../../src/rtlib/devices/dev_lpt.c"
#endif
#include FB_LPT_SOURCE
#undef calloc
#undef malloc
#undef free

FB_RTLIB_CTX fb_ctx;

FBCALL int fb_ErrorSetNum( int error ) { return error; }
int fb_DevLptClose( FB_FILE *handle ) { (void)handle; return 0; }
int fb_DevLptWrite( FB_FILE *handle, const void *data, size_t length )
{ (void)handle; (void)data; (void)length; return 0; }
int fb_DevLptWriteWstr( FB_FILE *handle, const FB_WCHAR *data, size_t length )
{ (void)handle; (void)data; (void)length; return 0; }
int fb_PrinterOpen( DEV_LPT_INFO *info, int port, const char *device )
{ (void)info; (void)port; (void)device; return printer_result; }

int fb_DevLptParseProtocol( DEV_LPT_PROTOCOL **result, const char *raw, size_t length, int substprn )
{
	DEV_LPT_PROTOCOL *protocol = calloc( 1, sizeof( *protocol ) );
	(void)raw; (void)length; (void)substprn;
	check( protocol != NULL );
	protocol->proto = "LPT1";
	protocol->name = "";
	protocol->iPort = 1;
	*result = protocol;
	return TRUE;
}

int main( void )
{
	DEV_LPT_PROTOCOL protocol = {0};
	DEV_LPT_INFO *shared;
	FB_FILE *existing = fb_ctx.fileTB + 3, *opened = fb_ctx.fileTB + 4;
	char *name;
	int i;

	protocol.proto = "LPT1";
	protocol.name = "named printer";
	name = fb_DevLptMakeDeviceName( &protocol );
	check( name != NULL && strcmp( name, "LPT1:named printer" ) == 0 );
	tracked_free( name );
	check( fb_DevLptMakeDeviceName( NULL ) == NULL );

	shared = tracked_calloc( 1, sizeof( *shared ) );
	check( shared != NULL );
	protocol.name = "";
	shared->pszDevice = fb_DevLptMakeDeviceName( &protocol );
	check( shared->pszDevice != NULL );
	shared->iPort = 1;
	shared->uiRefCount = 1;
	shared->driver_opaque = shared;
	existing->type = FB_FILE_TYPE_PRINTER;
	existing->hooks = &hooks_dev_lpt;
	existing->opaque = shared;
	for( i = 0; i < 100; ++i ) {
		check( fb_DevLptOpen( opened, "LPT1:", 5 ) == FB_RTERROR_OK );
		check( opened->opaque == shared && shared->uiRefCount == (size_t)i + 2 );
		check( live_allocations == 2 );
	}
	tracked_free( shared->pszDevice );
	tracked_free( shared );
	memset( existing, 0, sizeof( *existing ) );
	memset( opened, 0, sizeof( *opened ) );

	printer_result = FB_RTERROR_FILEIO;
	check( fb_DevLptOpen( opened, "LPT1:", 5 ) == FB_RTERROR_FILEIO );
	check( live_allocations == 0 && opened->opaque == NULL );
	for( i = 0; i < 2; ++i ) {
		allocation_calls = 0;
		fail_after = i;
		check( fb_DevLptOpen( opened, "LPT1:", 5 ) == FB_RTERROR_OUTOFMEM );
		check( live_allocations == 0 );
	}
	printf( "LPT_DEVICE_STATE_PASS checks=%d\n", checks );
	return 0;
}

/* end of lpt-device-state.c */
