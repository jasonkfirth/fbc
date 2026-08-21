/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/dev_tcp_test.c

    Purpose:

        Parse OPEN TCP device specifications on Windows CE.

    Responsibilities:

        - validate TCP option names and unsigned decimal values
        - reject overflowing values without depending on errno
        - build the protocol description consumed by the WinCE socket layer

    This file intentionally does NOT contain:

        - socket creation
        - DNS resolution
        - TCP stream I/O
*/

#include "../fb.h"
#include "fb_tcp_wince.h"

#include <ctype.h>
#include <limits.h>

static char *fb_hDevTcpTrim( char *p )
{
	char *end;

	while( isspace( (unsigned char)*p ) )
		++p;

	end = p + strlen( p );
	while( end > p ) {
		if( isspace( (unsigned char)end[-1] ) == 0 )
			break;
		--end;
	}
	*end = '\0';

	return p;
}

static int fb_hDevTcpParseUInt( const char *text, unsigned int *value )
{
	const unsigned char *cursor;
	unsigned int parsed = 0;

	if( text == NULL || value == NULL || *text == '\0' )
		return FALSE;

	for( cursor = (const unsigned char *)text; *cursor != '\0'; ++cursor ) {
		unsigned int digit;

		if( isdigit( *cursor ) == 0 )
			return FALSE;

		digit = (unsigned int)(*cursor - '0');
		if( parsed > (UINT_MAX - digit) / 10u )
			return FALSE;

		parsed = (parsed * 10u) + digit;
	}

	*value = (unsigned int)parsed;
	return TRUE;
}

int fb_DevTcpParseProtocol
	(
		DEV_TCP_PROTOCOL **tcp_proto_out,
		const char *proto_raw,
		size_t proto_raw_len,
		int is_server
	)
{
	char *p;
	DEV_TCP_PROTOCOL *tcp_proto;

	if( proto_raw == NULL )
		return FALSE;

	if( tcp_proto_out == NULL )
		return FALSE;

	*tcp_proto_out = calloc( sizeof( DEV_TCP_PROTOCOL ) + proto_raw_len + 2, 1 );
	tcp_proto = *tcp_proto_out;

	if( tcp_proto == NULL )
		return FALSE;

	memcpy( tcp_proto->raw, proto_raw, proto_raw_len );
	tcp_proto->raw[proto_raw_len] = '\0';

	tcp_proto->host = tcp_proto->raw + proto_raw_len;
	tcp_proto->port = 0;
	tcp_proto->timeout = 0;
	tcp_proto->backlog = 16;
	tcp_proto->is_server = is_server;

	p = tcp_proto->raw;
	while( *p ) {
		char *entry;
		char *comma;
		char *eq;
		char *key;
		char *value;
		unsigned int parsed;

		while( *p == ',' || isspace( (unsigned char)*p ) )
			++p;

		if( *p == '\0' )
			break;

		entry = p;
		comma = strchr( entry, ',' );
		if( comma != NULL ) {
			*comma = '\0';
			p = comma + 1;
		} else {
			p = entry + strlen( entry );
		}

		eq = strchr( entry, '=' );
		if( eq == NULL )
			return FALSE;

		*eq = '\0';
		key = fb_hDevTcpTrim( entry );
		value = fb_hDevTcpTrim( eq + 1 );

		if( strcasecmp( key, "host" ) == 0 ) {
			tcp_proto->host = value;
		} else if( strcasecmp( key, "port" ) == 0 ) {
			if( fb_hDevTcpParseUInt( value, &parsed ) == FALSE )
				return FALSE;
			if( parsed > 65535u )
				return FALSE;
			tcp_proto->port = parsed;
		} else if( strcasecmp( key, "timeout" ) == 0 ) {
			if( fb_hDevTcpParseUInt( value, &parsed ) == FALSE )
				return FALSE;
			tcp_proto->timeout = parsed;
		} else if( strcasecmp( key, "backlog" ) == 0 ) {
			if( fb_hDevTcpParseUInt( value, &parsed ) == FALSE )
				return FALSE;
			tcp_proto->backlog = parsed;
		} else {
			/* ignore unknown options for forward compatibility */
		}
	}

	if( tcp_proto->port == 0 )
		return FALSE;

	if( is_server == FALSE && (tcp_proto->host == NULL || *tcp_proto->host == '\0') )
		return FALSE;

	return TRUE;
}

/* end of wince/dev_tcp_test.c */
