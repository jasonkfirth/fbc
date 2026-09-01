/*
    FreeBASIC Runtime Library
    -------------------------

    File: file_dir.c

    Purpose:

        Implement the FreeBASIC Dir() function on desktop Windows.

    Responsibilities:

        - maintain per-thread directory enumeration state
        - select ANSI enumeration on Windows 95, 98 and ME
        - preserve Unicode enumeration on Windows NT systems
        - filter entries using FreeBASIC file attributes

    This file intentionally does NOT contain:

        - current-directory management
        - filesystem mutation
        - recursive directory traversal
*/

#include "../fb.h"
#ifndef HOST_CYGWIN
	#include <direct.h>
#endif
#include <windows.h>

/* ------------------------------------------------------------------------- */
/* Directory enumeration state                                               */
/* ------------------------------------------------------------------------- */

typedef struct {
	int in_use;
	int attrib;
	int return_utf8;
#ifdef HOST_CYGWIN
	WIN32_FIND_DATA data;
	HANDLE handle;
#else
	int use_ansi;
	WIN32_FIND_DATAA data_a;
	WIN32_FIND_DATAW data_w;
	HANDLE handle;
#endif
} FB_DIRCTX;

static void close_dir_internal ( FB_DIRCTX *ctx )
{
	FindClose( ctx->handle );
	ctx->in_use = FALSE;
}

void fb_DIRCTX_Destructor ( void* data )
{
	FB_DIRCTX *ctx = ( FB_DIRCTX *)data;
	if( ctx->in_use )
		close_dir_internal( ctx );
}

static void close_dir ( void )
{
	FB_DIRCTX *ctx = FB_TLSGETCTX( DIR );
	close_dir_internal( ctx );
}

#ifdef HOST_MINGW
static DWORD current_attributes( const FB_DIRCTX *ctx )
{
	if( ctx->use_ansi )
		return ctx->data_a.dwFileAttributes;

	return ctx->data_w.dwFileAttributes;
}

static char *copy_current_name( const FB_DIRCTX *ctx )
{
	if( ctx->use_ansi )
		return strdup( ctx->data_a.cFileName );

	return fb_hConvertPathFromWC( ctx->data_w.cFileName, ctx->return_utf8 );
}
#endif

/* ------------------------------------------------------------------------- */
/* Enumeration                                                               */
/* ------------------------------------------------------------------------- */

static char *find_next ( int *attrib )
{
	char *name = NULL;
	FB_DIRCTX *ctx = FB_TLSGETCTX( DIR );

#ifdef HOST_MINGW
	int handle_ok;

	do
	{
		if( ctx->use_ansi )
			handle_ok = FindNextFileA( ctx->handle, &ctx->data_a );
		else
			handle_ok = FindNextFileW( ctx->handle, &ctx->data_w );

		if( !handle_ok )
		{
			close_dir( );
			name = NULL;
			break;
		}
		name = copy_current_name( ctx );
		if( name != NULL && (current_attributes( ctx ) & ~ctx->attrib) ) {
			free( name );
			name = NULL;
		}
	}
	while( name == NULL && ctx->in_use );

	*attrib = current_attributes( ctx ) & ~0xFFFFFF00;

#else
    do {
        if( !FindNextFile( ctx->handle, &ctx->data ) ) {
            close_dir();
            name = NULL;
            break;
        }
        name = ctx->data.cFileName;
    } while( ctx->data.dwFileAttributes & ~ctx->attrib );

    *attrib = ctx->data.dwFileAttributes & ~0xFFFFFF00;
#endif

	return name;
}

FBCALL FBSTRING *fb_Dir( FBSTRING *filespec, int attrib, int *out_attrib )
{
	FB_DIRCTX *ctx;
	FBSTRING *res;
	ssize_t len;
	int tmp_attrib;
    char *name;
    int handle_ok;

	if( out_attrib == NULL )
		out_attrib = &tmp_attrib;

	len = FB_STRSIZE( filespec );
	name = NULL;

	ctx = FB_TLSGETCTX( DIR );

	if( len > 0 )
	{
		/* findfirst */
		if( ctx->in_use )
			close_dir( );

#ifdef HOST_MINGW
		ctx->use_ansi = fb_hWin32IsWin9x( );
		if( ctx->use_ansi ) {
			char *afilespec;

			ctx->return_utf8 = FALSE;
			afilespec = strdup( filespec->data );
			if( afilespec != NULL ) {
				fb_hConvertPath( afilespec );
				ctx->handle = FindFirstFileA( afilespec, &ctx->data_a );
				free( afilespec );
			} else {
				ctx->handle = FindFirstFileA( filespec->data, &ctx->data_a );
			}
			handle_ok = ctx->handle != INVALID_HANDLE_VALUE;
		} else {
			WCHAR *wfilespec;

			wfilespec = fb_hConvertPathToWC( filespec->data, &ctx->return_utf8 );
			if( wfilespec != NULL ) {
				ctx->handle = FindFirstFileW( wfilespec, &ctx->data_w );
				handle_ok = ctx->handle != INVALID_HANDLE_VALUE;
				free( wfilespec );
			} else {
				ctx->handle = INVALID_HANDLE_VALUE;
				handle_ok = FALSE;
			}
		}
#else
        ctx->handle = FindFirstFile( filespec->data, &ctx->data );
        handle_ok = ctx->handle != INVALID_HANDLE_VALUE;
#endif
		if( handle_ok )
		{
			/*
			   The search handle is live before filtering the first result.
			   Mark it owned here so find_next() can skip any number of
			   rejected entries and so every allocation-failure path closes
			   the handle.
			*/
			ctx->in_use = TRUE;

			/* Handle any other possible bits different Windows versions could return */
			ctx->attrib = attrib | 0xFFFFFF00;

			/* archive bit not set? set the dir bit at least.. */
			if( (attrib & 0x10) == 0 )
				ctx->attrib |= 0x20;

#ifdef HOST_MINGW
			if( current_attributes( ctx ) & ~ctx->attrib )
				name = find_next( out_attrib );
			else
			{
				name = copy_current_name( ctx );
				*out_attrib = current_attributes( ctx ) & ~0xFFFFFF00;
            }
#else
			if( ctx->data.dwFileAttributes & ~ctx->attrib )
				name = find_next( out_attrib );
			else
			{
                name = ctx->data.cFileName;
                *out_attrib = ctx->data.dwFileAttributes & ~0xFFFFFF00;
            }
#endif
			if( (name == NULL) && ctx->in_use )
				close_dir_internal( ctx );
		}
	} else {
		/* findnext */
		if( ctx->in_use )
			name = find_next( out_attrib );
	}

	FB_STRLOCK();

	/* store filename if found */
	if( name ) {
		len = strlen( name );
		res = fb_hStrAllocTemp_NoLock( NULL, len );
		if( res ) {
			fb_hStrCopy( res->data, name, len );
			#ifdef HOST_MINGW
			free( name );
			#endif
		} else {
			#ifdef HOST_MINGW
			free( name );
			#endif
			res = &__fb_ctx.null_desc;
		}
	} else {
		res = &__fb_ctx.null_desc;
		*out_attrib = 0;
	}

	fb_hStrDelTemp_NoLock( filespec );

	FB_STRUNLOCK();

	return res;
}

/* end of file_dir.c */
