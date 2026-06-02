/*
 * Xbox runtime environment support
 * --------------------------------
 *
 * File: sys_environ.c
 *
 * Purpose:
 *
 *     Provide ENVIRON and SETENVIRON support for the Xbox target.
 *
 * Responsibilities:
 *
 *     - store process-local environment variables
 *     - expose getenv() for C runtime users linked into FreeBASIC programs
 *     - implement FreeBASIC's ENVIRON and SETENVIRON entry points
 *
 * This file intentionally does NOT contain:
 *
 *     - host OS environment integration
 *     - persistence across program launches
 *     - platform configuration discovery
 */

#include "../fb.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------------- */
/* Environment storage                                                       */
/* ------------------------------------------------------------------------- */

/*
 * nxdk's current PDCLib getenv() implementation always returns NULL.  The
 * FreeBASIC runtime still needs a normal process-local environment so tests
 * and diagnostics can configure subsystems such as sfxlib after startup.
 */

typedef struct FB_XBOX_ENV_ENTRY {
	char *name;
	char *value;
	struct FB_XBOX_ENV_ENTRY *next;
} FB_XBOX_ENV_ENTRY;

static FB_XBOX_ENV_ENTRY *env_list = NULL;

static char *hStrDupLen( const char *text, size_t len )
{
	char *copy;

	copy = (char *)malloc( len + 1 );
	if( copy == NULL ) {
		errno = ENOMEM;
		return NULL;
	}

	memcpy( copy, text, len );
	copy[len] = '\0';

	return copy;
}

static FB_XBOX_ENV_ENTRY *hFindEnv( const char *name, size_t name_len )
{
	FB_XBOX_ENV_ENTRY *entry;

	for( entry = env_list; entry != NULL; entry = entry->next ) {
		if( (strlen( entry->name ) == name_len) &&
		    (memcmp( entry->name, name, name_len ) == 0) ) {
			return entry;
		}
	}

	return NULL;
}

static int hSetEnvPair( const char *name, size_t name_len, const char *value )
{
	FB_XBOX_ENV_ENTRY *entry;
	char *new_name;
	char *new_value;

	if( (name == NULL) || (name_len == 0) || (value == NULL) ) {
		errno = EINVAL;
		return -1;
	}

	new_value = hStrDupLen( value, strlen( value ) );
	if( new_value == NULL )
		return -1;

	entry = hFindEnv( name, name_len );
	if( entry != NULL ) {
		free( entry->value );
		entry->value = new_value;
		return 0;
	}

	new_name = hStrDupLen( name, name_len );
	if( new_name == NULL ) {
		free( new_value );
		return -1;
	}

	entry = (FB_XBOX_ENV_ENTRY *)calloc( 1, sizeof( FB_XBOX_ENV_ENTRY ) );
	if( entry == NULL ) {
		free( new_name );
		free( new_value );
		errno = ENOMEM;
		return -1;
	}

	entry->name = new_name;
	entry->value = new_value;
	entry->next = env_list;
	env_list = entry;

	return 0;
}

static int hUnsetEnvName( const char *name, size_t name_len )
{
	FB_XBOX_ENV_ENTRY **link;

	if( (name == NULL) || (name_len == 0) ) {
		errno = EINVAL;
		return -1;
	}

	link = &env_list;
	while( *link != NULL ) {
		FB_XBOX_ENV_ENTRY *entry = *link;

		if( (strlen( entry->name ) == name_len) &&
		    (memcmp( entry->name, name, name_len ) == 0) ) {
			*link = entry->next;
			free( entry->name );
			free( entry->value );
			free( entry );
			return 0;
		}

		link = &entry->next;
	}

	return 0;
}

static int hSetEnvironText( const char *text )
{
	const char *equals;

	if( text == NULL ) {
		errno = EINVAL;
		return -1;
	}

	equals = strchr( text, '=' );
	if( equals == NULL )
		return hUnsetEnvName( text, strlen( text ) );

	return hSetEnvPair( text, (size_t)(equals - text), equals + 1 );
}

/* ------------------------------------------------------------------------- */
/* C environment API                                                         */
/* ------------------------------------------------------------------------- */

char *getenv( const char *name )
{
	FB_XBOX_ENV_ENTRY *entry;
	size_t name_len;

	if( (name == NULL) || (name[0] == '\0') || (strchr( name, '=' ) != NULL) )
		return NULL;

	name_len = strlen( name );
	entry = hFindEnv( name, name_len );

	return (entry != NULL) ? entry->value : NULL;
}

int setenv( const char *name, const char *value, int overwrite )
{
	if( (name == NULL) || (name[0] == '\0') || (strchr( name, '=' ) != NULL) ||
	    (value == NULL) ) {
		errno = EINVAL;
		return -1;
	}

	if( (overwrite == 0) && (hFindEnv( name, strlen( name ) ) != NULL) )
		return 0;

	return hSetEnvPair( name, strlen( name ), value );
}

int unsetenv( const char *name )
{
	if( (name == NULL) || (name[0] == '\0') || (strchr( name, '=' ) != NULL) ) {
		errno = EINVAL;
		return -1;
	}

	return hUnsetEnvName( name, strlen( name ) );
}

/* ------------------------------------------------------------------------- */
/* FreeBASIC runtime API                                                     */
/* ------------------------------------------------------------------------- */

FBCALL FBSTRING *fb_GetEnviron( FBSTRING *varname )
{
	FBSTRING *dst;
	char *tmp;
	ssize_t len;

	if( (varname != NULL) && (varname->data != NULL) )
		tmp = getenv( varname->data );
	else
		tmp = NULL;

	FB_STRLOCK();

	if( tmp != NULL ) {
		len = strlen( tmp );
		dst = fb_hStrAllocTemp_NoLock( NULL, len );
		if( dst != NULL )
			fb_hStrCopy( dst->data, tmp, len );
		else
			dst = &__fb_ctx.null_desc;
	} else {
		dst = &__fb_ctx.null_desc;
	}

	/* del if temp */
	fb_hStrDelTemp_NoLock( varname );

	FB_STRUNLOCK();

	return dst;
}

FBCALL int fb_SetEnviron( FBSTRING *str )
{
	int res = 0;

	if( (str != NULL) && (str->data != NULL) )
		res = hSetEnvironText( str->data );

	/* del if temp */
	fb_hStrDelTemp( str );

	return res;
}

/* end of sys_environ.c */
