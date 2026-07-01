/* SHELL command */

#include "../fb.h"

int fb_hShell( char *program )
{
	/* SHELL is specified as a command-backed runtime call. */
	// NOLINTNEXTLINE(bugprone-command-processor)
	return system( program );
}
