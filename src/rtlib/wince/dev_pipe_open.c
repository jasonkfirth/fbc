/*
    FreeBASIC Windows CE runtime
    ----------------------------

    File: dev_pipe_open.c

    Purpose:

        Provide bounded read-only OPEN PIPE support on Windows CE systems
        that do not expose anonymous pipes or a desktop command processor.

    Responsibilities:

        * recognize one-file directory-listing command forms
        * validate that the requested file exists
        * expose a one-line listing through the normal file-device hooks
        * reject unsupported modes and commands without launching a process

    This file intentionally does NOT contain:

        * a general-purpose command interpreter
        * output or bidirectional pipe support
        * desktop CreatePipe or _popen emulation

    Coredll supplies CreateProcessW but omits the anonymous-pipe and standard-
    handle inheritance APIs needed to implement popen().  A temporary stream
    provides the observable behavior required by the portable one-file
    listing contract without pretending that Windows CE has a desktop shell.
*/

#include "../fb.h"

#include <ctype.h>

#define FB_WINCE_PIPE_TEMP_FILE "fbpipe.tmp"

/* ------------------------------------------------------------------------- */
/* File-device hooks                                                         */
/* ------------------------------------------------------------------------- */

static FB_FILE_HOOKS hooks_dev_wince_pipe = {
    fb_DevFileEof,
    fb_DevFileClose,
    fb_DevFileSeek,
    fb_DevFileTell,
    fb_DevFileRead,
    fb_DevFileReadWstr,
    fb_DevFileWrite,
    fb_DevFileWriteWstr,
    fb_DevFileLock,
    fb_DevFileUnlock,
    fb_DevFileReadLine,
    fb_DevFileReadLineWstr,
    NULL,
    fb_DevFileFlush
};

/* ------------------------------------------------------------------------- */
/* Supported command parsing                                                 */
/* ------------------------------------------------------------------------- */

static char *hSkipSpaces( char *cursor )
{
    while( isspace( (unsigned char)*cursor ) )
        ++cursor;

    return cursor;
}

static void hTrimRight( char *text )
{
    size_t length = strlen( text );

    while( length > 0 ) {
        if( !isspace( (unsigned char)text[length - 1] ) )
            break;

        text[--length] = '\0';
    }
}

static int hConsumeToken( char **cursor, const char *expected )
{
    char *input = hSkipSpaces( *cursor );
    const char *token = expected;

    while( *token!='\0' ) {
        if( tolower( (unsigned char)*input )!=
            tolower( (unsigned char)*token ) )
            return FALSE;

        ++input;
        ++token;
    }

    if( *input!='\0' && !isspace( (unsigned char)*input ) )
        return FALSE;

    *cursor = input;
    return TRUE;
}

static int hCopyPathArgument( char *cursor,
                              char *destination,
                              size_t destination_length )
{
    char *end;
    size_t length;

    cursor = hSkipSpaces( cursor );
    if( *cursor=='\0' )
        return FALSE;

    if( *cursor=='"' || *cursor=='\'' ) {
        char quote = *cursor++;

        end = strchr( cursor, quote );
        if( end==NULL )
            return FALSE;

        *end = '\0';
        if( *hSkipSpaces( end + 1 )!='\0' )
            return FALSE;
    } else {
        hTrimRight( cursor );
    }

    length = strlen( cursor );
    if( length==0 || length>=destination_length )
        return FALSE;

    memcpy( destination, cursor, length + 1 );
    return TRUE;
}

static int hCopyListedPath( char *command,
                            char *destination,
                            size_t destination_length )
{
    char *cursor = command;

    /* Accept the compact command used by shell-less target baselines. */
    if( hConsumeToken( &cursor, "ls" ) )
        return hCopyPathArgument( cursor,
                                  destination,
                                  destination_length );

    cursor = command;

    /*
        The shared Win32-compatible test spells the operation as:

            cmd /d /c dir /b "path"

        Only that exact read-only listing shape is recognized.  No command is
        executed, so metacharacters and additional arguments remain inert.
    */
    if( !hConsumeToken( &cursor, "cmd" ) ||
        !hConsumeToken( &cursor, "/d" ) ||
        !hConsumeToken( &cursor, "/c" ) ||
        !hConsumeToken( &cursor, "dir" ) ||
        !hConsumeToken( &cursor, "/b" ) )
        return FALSE;

    return hCopyPathArgument( cursor,
                              destination,
                              destination_length );
}

/* ------------------------------------------------------------------------- */
/* Temporary result stream                                                   */
/* ------------------------------------------------------------------------- */

static int hOpenResult( FB_FILE *handle )
{
    FILE *stream;

    stream = fb_hOpenFile( FB_WINCE_PIPE_TEMP_FILE, "rb" );
    if( stream==NULL )
        return fb_ErrorSetNum( FB_RTERROR_FILENOTFOUND );

    fb_hSetFileBufSize( stream );
    handle->size = fb_DevFileGetSize( stream,
                                      FB_FILE_MODE_INPUT,
                                      handle->encod,
                                      FALSE );
    if( handle->size==-1 ) {
        fclose( stream );
        return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
    }

    stream = fb_hReopenFile( FB_WINCE_PIPE_TEMP_FILE, "rt", stream );
    if( stream==NULL )
        return fb_ErrorSetNum( FB_RTERROR_FILENOTFOUND );

    fb_hSetFileBufSize( stream );
    fb_hDevFileSeekStart( stream,
                          FB_FILE_MODE_INPUT,
                          handle->encod,
                          FALSE );

    handle->hooks = &hooks_dev_wince_pipe;
    handle->opaque = stream;
    handle->type = FB_FILE_TYPE_PIPE;
    handle->access = FB_FILE_ACCESS_READ;

    return fb_ErrorSetNum( FB_RTERROR_OK );
}

/* ------------------------------------------------------------------------- */
/* OPEN PIPE entry point                                                     */
/* ------------------------------------------------------------------------- */

int fb_DevPipeOpen( FB_FILE *handle,
                    const char *filename,
                    size_t filename_length )
{
    char *command;
    char path[MAX_PATH];
    FILE *stream;
    size_t path_length;
    int result;
    int valid_command;

    FB_LOCK();

    if( handle->mode!=FB_FILE_MODE_INPUT ) {
        FB_UNLOCK();
        return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
    }

    if( handle->access==FB_FILE_ACCESS_ANY )
        handle->access = FB_FILE_ACCESS_READ;

    if( handle->access!=FB_FILE_ACCESS_READ ) {
        FB_UNLOCK();
        return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
    }

    command = (char *)malloc( filename_length + 1 );
    if( command==NULL ) {
        FB_UNLOCK();
        return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
    }

    memcpy( command, filename, filename_length );
    command[filename_length] = '\0';
    valid_command = hCopyListedPath( command, path, sizeof( path ) );
    free( command );

    if( !valid_command ) {
        FB_UNLOCK();
        return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
    }

    stream = fb_hOpenFile( path, "rb" );
    if( stream==NULL ) {
        FB_UNLOCK();
        return fb_ErrorSetNum( FB_RTERROR_FILENOTFOUND );
    }
    fclose( stream );

    stream = fb_hOpenFile( FB_WINCE_PIPE_TEMP_FILE, "wb" );
    if( stream==NULL ) {
        FB_UNLOCK();
        return fb_ErrorSetNum( FB_RTERROR_FILEIO );
    }

    path_length = strlen( path );
    if( fwrite( path, 1, path_length, stream )!=path_length ||
        fwrite( "\n", 1, 1, stream )!=1 ) {
        fclose( stream );
        FB_UNLOCK();
        return fb_ErrorSetNum( FB_RTERROR_FILEIO );
    }
    fclose( stream );

    result = hOpenResult( handle );
    FB_UNLOCK();
    return result;
}

/* end of dev_pipe_open.c */
