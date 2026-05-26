#include "../fb.h"
#include "fb_private_console.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

FBCALL int fb_ExecEx( FBSTRING *program, FBSTRING *args, int do_fork )
{
	char buffer[MAX_PATH], *arguments, **argv, *p;
	char empty_arguments[] = "";
	int i, argc = 0, res = -1, status;
	ssize_t len_arguments;
	pid_t pid;

	if( (program == NULL) || (program->data == NULL) ) 
	{
		fb_hStrDelTemp( args );
		fb_hStrDelTemp( program );
		return -1;
	}

	strncpy( buffer, program->data, sizeof( buffer ) );
	buffer[sizeof( buffer ) - 1] = 0;

	fb_hConvertPath( buffer );

	if( args==NULL ) {
		arguments = empty_arguments;
	} else {
		len_arguments = FB_STRSIZE( args );
		arguments = alloca( len_arguments + 1 );
		DBG_ASSERT( arguments!=NULL );
		arguments[len_arguments] = 0;
		if( len_arguments )
			argc = fb_hParseArgs( arguments, args->data, len_arguments );
	}

	FB_STRLOCK();

	fb_hStrDelTemp_NoLock( args );
	fb_hStrDelTemp_NoLock( program );

	FB_STRUNLOCK();

	if( argc == -1 )
		return -1;

	argc++; 			/* add 1 for program name */

	argv = alloca( sizeof(char*) * (argc + 1 ));
	DBG_ASSERT( argv!=NULL );

	argv[0] = buffer;

	/* scan the processed args and set pointers */
	p = arguments;
	for( i=1 ; i<argc; i++) {
		argv[i] = p;	/* set pointer to current argument */
		while( *p++ );	/* skip to 1 char past next null char */
	}
	argv[argc] = NULL;


	/* Launch */
	FB_LOCK( );
	fb_hExitConsole();
	FB_UNLOCK( );

	if( do_fork ) {
		int exec_status_pipe[2] = { -1, -1 };

		if( pipe( exec_status_pipe ) != 0 ) {
			exec_status_pipe[0] = -1;
			exec_status_pipe[1] = -1;
		} else {
			fcntl( exec_status_pipe[0], F_SETFD, FD_CLOEXEC );
			fcntl( exec_status_pipe[1], F_SETFD, FD_CLOEXEC );
		}

		pid = fork();
		if( pid != -1 ) {
			if (pid == 0) {
				if( exec_status_pipe[0] != -1 )
					close( exec_status_pipe[0] );

				/* execvp() only returns if it failed */
				execvp( buffer, argv );

				if( exec_status_pipe[1] != -1 ) {
					int exec_errno = errno;
					write( exec_status_pipe[1], &exec_errno, sizeof( exec_errno ) );
					close( exec_status_pipe[1] );
				}

				/* Using _exit() instead of exit() to prevent the child from flusing file I/O and
				   running global destructors (especially the rtlib's own cleanup), which may try
				   to wait on threads to finish (e.g. hinit.c::bg_thread()), but fork() doesn't
				   duplicate other threads besides the current one, so their pthread handles will be
				   invalid here in the child process. */
				_exit( 255 );
			} else if( (waitpid(pid, &status, 0) > 0) && WIFEXITED(status) ) {
				int exec_errno = 0;
				ssize_t exec_errno_size = 0;

				if( exec_status_pipe[1] != -1 )
					close( exec_status_pipe[1] );
				exec_status_pipe[1] = -1;

				if( exec_status_pipe[0] != -1 )
					exec_errno_size = read( exec_status_pipe[0], &exec_errno, sizeof( exec_errno ) );

				if( exec_errno_size == sizeof( exec_errno ) )
					res = -1;
				else
					res = WEXITSTATUS(status);
			}
		}

		if( exec_status_pipe[0] != -1 )
			close( exec_status_pipe[0] );
		if( exec_status_pipe[1] != -1 )
			close( exec_status_pipe[1] );
	} else {
		res = execvp( buffer, argv );
	}

	FB_LOCK( );
	fb_hInitConsole();
	FB_UNLOCK( );

	return res;
}
