/*
 * Project: FreeBASIC library diagnostics
 * File: capture-library-compiler.c
 * Purpose: Capture each compiler invocation's diagnostics beside its object.
 * Responsibilities: Forward arguments, retain diagnostics and return compiler status.
 * This file intentionally does NOT contain: compiler flags or source selection.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <io.h>
#include <process.h>
#define audit_open _open
#define audit_close _close
#define audit_dup _dup
#define audit_dup2 _dup2
#define AUDIT_BINARY _O_BINARY
#define AUDIT_MODE (_S_IREAD | _S_IWRITE)
#else
#include <sys/wait.h>
#include <unistd.h>
#define audit_open open
#define audit_close close
#define audit_dup dup
#define audit_dup2 dup2
#define AUDIT_BINARY 0
#define AUDIT_MODE (S_IRUSR | S_IWUSR)
#endif

#ifdef _WIN32
/* _spawnv joins arguments into a command line. Quote each one for the child
   CRT so embedded quotes, spaces and trailing backslashes survive unchanged. */
static char *quote_argument( const char *argument )
{
    size_t length = strlen( argument ), slashes = 0;
    char *quoted, *out;
    if( length > (SIZE_MAX - 3) / 2 )
        return NULL;
    quoted = malloc( length * 2 + 3 );
    if( quoted == NULL )
        return NULL;
    out = quoted;
    *out++ = '"';
    for( ;; ++argument ) {
        if( *argument == '\\' ) {
            ++slashes;
            continue;
        }
        if( *argument == '"' || *argument == '\0' )
            slashes *= 2;
        while( slashes != 0 ) {
            *out++ = '\\';
            --slashes;
        }
        if( *argument == '\0' )
            break;
        if( *argument == '"' )
            *out++ = '\\';
        *out++ = *argument;
    }
    *out++ = '"';
    *out = '\0';
    return quoted;
}
#endif

static int invoke_compiler( const char *compiler, int argc, char **arguments )
{
#ifdef _WIN32
    char **quoted = calloc( (size_t)argc + 1, sizeof( *quoted ) );
    int i, status = -1, saved_errno = ENOMEM;
    if( quoted == NULL )
        return -1;
    for( i = 0; i < argc; ++i ) {
        quoted[i] = quote_argument( arguments[i] );
        if( quoted[i] == NULL )
            goto cleanup;
    }
    status = (int)_spawnv( _P_WAIT, compiler, (const char *const *)quoted );
    saved_errno = errno;
cleanup:
    for( i = 0; i < argc; ++i )
        free( quoted[i] );
    free( quoted );
    errno = saved_errno;
    return status;
#else
    int status;
    pid_t child = fork();
    (void)argc;
    if( child < 0 )
        return -1;
    if( child == 0 ) {
        execv( compiler, arguments );
        _exit( 127 );
    }
    while( waitpid( child, &status, 0 ) < 0 ) {
        if( errno != EINTR )
            return -1;
    }
    if( WIFEXITED( status ) )
        return WEXITSTATUS( status );
    return 128 + WTERMSIG( status );
#endif
}

int main( int argc, char **argv )
{
    char *compiler = getenv( "FBC_AUDIT_REAL_CC" );
    char **arguments = NULL;
    char *capture_path = NULL;
    const char *object = NULL;
    int compile = 0, i, capture_fd = -1, saved_out = -1, saved_err = -1;
    int status = 2, invoked = 0, spawn_error = 0;
    size_t length;

    if( compiler == NULL || compiler[0] == '\0' ) {
        fputs( "FBC_AUDIT_REAL_CC must name the real compiler\n", stderr );
        return 2;
    }
    arguments = calloc( (size_t)argc + 1, sizeof( *arguments ) );
    if( arguments == NULL )
        return 2;
    arguments[0] = compiler;
    for( i = 1; i < argc; ++i ) {
        arguments[i] = argv[i];
        if( strcmp( argv[i], "-c" ) == 0 )
            compile = 1;
        if( strcmp( argv[i], "-o" ) == 0 && i + 1 < argc )
            object = argv[i + 1];
    }
    /* Version, preprocessing and feature probes retain normal driver behavior.
       The Make-selected object invocations get independent diagnostic files. */
    if( compile && object != NULL ) {
        length = strlen( object );
        if( length >= 2 && strcmp( object + length - 2, ".o" ) == 0 ) {
            if( length > SIZE_MAX - sizeof( ".diagnostics" ) )
                goto cleanup;
            capture_path = malloc( length + sizeof( ".diagnostics" ) );
            if( capture_path == NULL )
                goto cleanup;
            memcpy( capture_path, object, length );
            memcpy( capture_path + length, ".diagnostics", sizeof( ".diagnostics" ) );
            capture_fd = audit_open( capture_path, O_CREAT | O_TRUNC | O_WRONLY | AUDIT_BINARY, AUDIT_MODE );
            if( capture_fd < 0 )
                goto cleanup;
            saved_out = audit_dup( 1 );
            saved_err = audit_dup( 2 );
            if( saved_out < 0 || saved_err < 0 )
                goto cleanup;
            if( audit_dup2( capture_fd, 1 ) < 0 || audit_dup2( capture_fd, 2 ) < 0 )
                goto cleanup;
        }
    }
    status = invoke_compiler( compiler, argc, arguments );
    invoked = 1;
    if( status == -1 ) {
        spawn_error = errno;
        status = 2;
    }

cleanup:
    if( saved_out >= 0 ) {
        audit_dup2( saved_out, 1 );
        audit_close( saved_out );
    }
    if( saved_err >= 0 ) {
        audit_dup2( saved_err, 2 );
        audit_close( saved_err );
    }
    if( capture_fd >= 0 )
        audit_close( capture_fd );
    if( invoked && capture_path != NULL ) {
        FILE *diagnostics = fopen( capture_path, "rb" );
        if( diagnostics != NULL ) {
            char buffer[4096];
            size_t bytes;
            while( (bytes = fread( buffer, 1, sizeof( buffer ), diagnostics )) != 0 ) {
                if( fwrite( buffer, 1, bytes, stderr ) != bytes ) {
                    status = 2;
                    break;
                }
            }
            if( ferror( diagnostics ) )
                status = 2;
            fclose( diagnostics );
        } else {
            status = 2;
        }
    }
    if( spawn_error != 0 )
        fprintf( stderr, "Cannot start compiler: %s\n", strerror( spawn_error ) );
    if( !invoked )
        fputs( "Cannot prepare compiler diagnostic capture\n", stderr );
    free( capture_path );
    free( arguments );
    return status;
}

/* end of capture-library-compiler.c */
