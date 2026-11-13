/*
 * Project: FreeBASIC runtime library
 * File: hooks/hook_ports.c
 * Purpose: ports I/O hooks, default to system implementations.
 * Responsibilities: Dispatch between console services and installed graphics hooks.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL int fb_In( unsigned short port )
{
	int res = -1;
	
	FB_LOCK();
	
	if( fb_ctx.hooks.inproc)
		res = fb_ctx.hooks.inproc( port );
	if( res < 0 )
		res = fb_hIn( port );
	
	FB_UNLOCK();
	
	return res;
}

/*:::::*/
FBCALL int fb_Out( unsigned short port, unsigned char value )
{
	int res = -1;
	
	FB_LOCK();
	
	if( fb_ctx.hooks.outproc)
		res = fb_ctx.hooks.outproc( port, value );
	if( res < 0 )
		res = fb_hOut( port, value );
	
	FB_UNLOCK();
	
	return res;
}

/*:::::*/
FBCALL int fb_Wait( unsigned short port, int andMask, int xorMask )
{
	int res;
	
	do {
		res = fb_In( port );
		if( res < 0 )
			return res;
		res ^= xorMask;
	} while( ( res & andMask ) == 0 );
	
	return FB_RTERROR_OK;
}

/* end of hooks/hook_ports.c */
