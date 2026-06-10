/* LPTx device */

#include "fb.h"
#include <strings.h>
#include <ctype.h>
#include <limits.h>

static int fb_hDevLptParsePort( const char *proto, int *port )
{
	const char *digits;
	int value;

	if( port == NULL )
		return FALSE;

	*port = 0;

	if( strncasecmp( proto, "LPT", 3 ) != 0 )
		return FALSE;

	digits = proto + 3;
	if( *digits == '\0' )
		return TRUE;

	value = 0;
	while( *digits )
	{
		int digit;

		if( ( *digits < '0' ) || ( *digits > '9' ) )
			return FALSE;

		digit = *digits - '0';
		if( value > (INT_MAX - digit) / 10 )
			return FALSE;

		value = (value * 10) + digit;
		++digits;
	}

	if( value <= 0 )
		return FALSE;

	*port = value;
	return TRUE;
}

/** Tests for the right file name for LPT access.
 *
 * Allowed file names are:
 *
 * - PRN:
 * - LPT:
 * - LPTx: with x>=1
 * - LPT:printer_name,EMU=?,TITLE=?,OPT=?, ...
 */

int fb_DevLptParseProtocol
	(
		DEV_LPT_PROTOCOL ** lpt_proto_out,
		const char * proto_raw,
		size_t proto_raw_len,
		int subst_prn
	)
{
	char *p, *ptail, *pc, *pe;
	DEV_LPT_PROTOCOL *lpt_proto;

	if( proto_raw == NULL )
		return FALSE;

	if( lpt_proto_out == NULL )
		return FALSE;

	*lpt_proto_out = calloc( sizeof( DEV_LPT_PROTOCOL ) + proto_raw_len + 2, 1 );
	lpt_proto = *lpt_proto_out;

	if( lpt_proto == NULL )
		return FALSE;

	strncpy( lpt_proto->raw, proto_raw, proto_raw_len );
	lpt_proto->raw[proto_raw_len] = '\0';

	p = lpt_proto->raw;
	ptail = p + strlen( lpt_proto->raw );

	lpt_proto->iPort = 0;
	lpt_proto->proto =
	  lpt_proto->name = 
	    lpt_proto->title =
	      lpt_proto->emu = ptail;

	/* "PRN:" */

		if( strcasecmp( p, "PRN:" ) == 0)
		{
			if( subst_prn )
				memcpy( p, "LPT1:", sizeof( "LPT1:" ) );

			lpt_proto->proto = p;
		lpt_proto->iPort = 1;
		return TRUE;
	}

	/* "LPTx:" */
	
	if( strncasecmp( p, "LPT", 3) != 0)
		return FALSE;

	pc = strchr( p, ':' );
	if( !pc )
		return FALSE;

	lpt_proto->proto = p;
	p = pc + 1;
	*pc = '\0';

	/* Get port number if any */
	if( fb_hDevLptParsePort( lpt_proto->proto, &lpt_proto->iPort ) == FALSE )
		return FALSE;

	/* Name, TITLE=?, EMU=? */

	while( *p )
	{
		if( isspace( FB_CHAR_TO_INT( *p ) ) || ( *p == ',' ))
			p++;

		else
		{
			char * pt;

			pe = strchr(p, '=');
			pc = strchr(p, ',');

			if( pc && pe && pe > pc )
				pe = NULL;

			if( !pe )
			{
				lpt_proto->name = p;
			}
			else
			{
				/* remove spaces before '=' */
				pt = pe;
				while( (pt > p) && isspace( FB_CHAR_TO_INT( *(pt - 1) ) ) )
				{
					--pt;
					*pt = '\0';
				}

				/* remove spaces after '=' or end*/
				*pe++ = '\0';
				while( isspace( FB_CHAR_TO_INT( *pe ) )) *pe++ = '\0';

				if( strcasecmp( p, "EMU" ) == 0)
				{
					lpt_proto->emu = pe;
				}
				else if( strcasecmp( p, "TITLE" ) == 0)
				{
					lpt_proto->title = pe;
				}
				/* just ignore options we don't understand to allow forward compatibility */
			}

			/* remove spaces before ',' or end*/
			pt = pc ? pc : ptail;
			while( (pt > p) && isspace( FB_CHAR_TO_INT( *(pt - 1) ) ) )
			{
				--pt;
				*pt = '\0';
			}

			if( pc )
			{
				p = pc + 1;
				*pc = '\0';
			}	
			else
			{
				p = ptail;
			}
		}
	}

	return TRUE;
}

int fb_DevLptTestProtocol( FB_FILE *handle, const char *filename, size_t filename_len )
{
	DEV_LPT_PROTOCOL *lpt_proto = NULL;
	int ret = fb_DevLptParseProtocol( &lpt_proto, filename, filename_len, FALSE );
	if( lpt_proto )
		free( lpt_proto );
	return ret;
}
