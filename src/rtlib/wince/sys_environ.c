/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/sys_environ.c

    Purpose:

        Provide process-local ENVIRON and SETENVIRON support on Windows CE.

    Responsibilities:

        - store environment name/value pairs for the current process
        - return values through FreeBASIC temporary strings
        - remove variables when SETENVIRON receives a name without '='

    This file intentionally does NOT contain:

        - desktop Windows environment-block integration
        - persistence across program launches
        - CeGCC's getenv() stub
*/

#include "../fb.h"

#include <windows.h>

/* ------------------------------------------------------------------------- */
/* Environment storage                                                       */
/* ------------------------------------------------------------------------- */

/*
    Windows CE has no conventional process environment.  CeGCC consequently
    provides getenv() as an inline function that always returns NULL.  Keep a
    private environment so FreeBASIC programs and target backends can still
    exchange process-local configuration in the usual way.
*/

typedef struct FB_WINCE_ENV_ENTRY {
	char *name;
	char *value;
	struct FB_WINCE_ENV_ENTRY *next;
} FB_WINCE_ENV_ENTRY;

static FB_WINCE_ENV_ENTRY *env_list = NULL;

static char *hDuplicateText( const char *text, size_t length )
{
	char *copy;

	copy = (char *)malloc( length + 1 );
	if( copy == NULL ) {
		SetLastError( ERROR_NOT_ENOUGH_MEMORY );
		return NULL;
	}

	memcpy( copy, text, length );
	copy[length] = '\0';

	return copy;
}

static FB_WINCE_ENV_ENTRY *hFindEntry( const char *name, size_t name_length )
{
	FB_WINCE_ENV_ENTRY *entry;

	for( entry = env_list; entry != NULL; entry = entry->next ) {
		if( (strlen( entry->name ) == name_length) &&
		    (memcmp( entry->name, name, name_length ) == 0) ) {
			return entry;
		}
	}

	return NULL;
}

static const char *hGetValue( const char *name )
{
	FB_WINCE_ENV_ENTRY *entry;

	if( (name == NULL) || (name[0] == '\0') ||
	    (strchr( name, '=' ) != NULL) ) {
		return NULL;
	}

	entry = hFindEntry( name, strlen( name ) );
	return (entry != NULL) ? entry->value : NULL;
}

static int hSetPair( const char *name, size_t name_length, const char *value )
{
	FB_WINCE_ENV_ENTRY *entry;
	char *new_name;
	char *new_value;

	if( (name == NULL) || (name_length == 0) || (value == NULL) ) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return -1;
	}

	new_value = hDuplicateText( value, strlen( value ) );
	if( new_value == NULL )
		return -1;

	entry = hFindEntry( name, name_length );
	if( entry != NULL ) {
		free( entry->value );
		entry->value = new_value;
		return 0;
	}

	new_name = hDuplicateText( name, name_length );
	if( new_name == NULL ) {
		free( new_value );
		return -1;
	}

	entry = (FB_WINCE_ENV_ENTRY *)calloc( 1, sizeof( FB_WINCE_ENV_ENTRY ) );
	if( entry == NULL ) {
		free( new_name );
		free( new_value );
		SetLastError( ERROR_NOT_ENOUGH_MEMORY );
		return -1;
	}

	entry->name = new_name;
	entry->value = new_value;
	entry->next = env_list;
	env_list = entry;

	return 0;
}

static int hRemoveName( const char *name, size_t name_length )
{
	FB_WINCE_ENV_ENTRY **link;

	if( (name == NULL) || (name_length == 0) ) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return -1;
	}

	link = &env_list;
	while( *link != NULL ) {
		FB_WINCE_ENV_ENTRY *entry = *link;

		if( (strlen( entry->name ) == name_length) &&
		    (memcmp( entry->name, name, name_length ) == 0) ) {
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

static int hSetText( const char *text )
{
	const char *equals;

	if( text == NULL ) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return -1;
	}

	equals = strchr( text, '=' );
	if( equals == NULL )
		return hRemoveName( text, strlen( text ) );

	return hSetPair( text, (size_t)(equals - text), equals + 1 );
}

/* ------------------------------------------------------------------------- */
/* Platform API                                                              */
/* ------------------------------------------------------------------------- */

const char *fb_hWinCEGetEnv( const char *name )
{
	return hGetValue( name );
}

/* ------------------------------------------------------------------------- */
/* FreeBASIC runtime API                                                     */
/* ------------------------------------------------------------------------- */

FBCALL FBSTRING *fb_GetEnviron( FBSTRING *varname )
{
	FBSTRING *result;
	const char *value;
	ssize_t length;

	FB_LOCK();

	if( (varname != NULL) && (varname->data != NULL) )
		value = hGetValue( varname->data );
	else
		value = NULL;

	FB_STRLOCK();

	if( value != NULL ) {
		length = strlen( value );
		result = fb_hStrAllocTemp_NoLock( NULL, length );
		if( result != NULL )
			fb_hStrCopy( result->data, value, length );
		else
			result = &__fb_ctx.null_desc;
	} else {
		result = &__fb_ctx.null_desc;
	}

	/* Delete the descriptor when the caller supplied a temporary string. */
	fb_hStrDelTemp_NoLock( varname );

	FB_STRUNLOCK();
	FB_UNLOCK();

	return result;
}

FBCALL int fb_SetEnviron( FBSTRING *str )
{
	int result = 0;

	FB_LOCK();

	if( (str != NULL) && (str->data != NULL) )
		result = hSetText( str->data );

	FB_UNLOCK();

	/* Delete the descriptor when the caller supplied a temporary string. */
	fb_hStrDelTemp( str );

	return result;
}

/* end of wince/sys_environ.c */
