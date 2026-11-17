/*
 * Project: FreeBASIC Unix runtime
 * File: fb_private_termcap.h
 * Purpose: Adapt two-character capability names to legacy termcap interfaces.
 * Responsibilities: Supply writable, terminated capability IDs for each call.
 * Console initialization and key translation belong to their driver modules.
 */
#ifndef FB_PRIVATE_TERMCAP_H
#define FB_PRIVATE_TERMCAP_H

#include <termcap.h>

/* Older termcap headers accept char * IDs. A two-byte ID plus its terminator
   fits in local storage, leaving the caller's literal or table unchanged.
   The lookup consumes the ID during the call and returns separate storage. */
static __inline__ char *fb_hTermcapGetStr( const char *id, char **area )
{
	char capability[3] = { id[0], id[1], '\0' };
	return tgetstr( capability, area );
}

static __inline__ int fb_hTermcapGetFlag( const char *id )
{
	char capability[3] = { id[0], id[1], '\0' };
	return tgetflag( capability );
}

#endif

/* end of fb_private_termcap.h */
