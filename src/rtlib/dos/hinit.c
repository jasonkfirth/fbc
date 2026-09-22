/* FreeBASIC DOS runtime: hinit.c
 * Initialize process state before BASIC global constructors, including the
 * optional TCP clock profile. This does not initialize graphics or networking.
 */

#include "../fb.h"
#include "fb_private_console.h"
#include "../fb_private_thread.h"
#include "dev_tcp_watt.h"
#include <float.h>
#include <unistd.h>
#include <conio.h>

FB_CONSOLE_CTX __fb_con;
char *__fb_startup_cwd;

void fb_hInit( void )
{

	/*
	 * DOSBox/DPMI hosts can leave x87 tag state behind between short-lived
	 * compiler invocations.  Start every DOS program with an empty x87 stack
	 * before applying the FreeBASIC control word.
	 */
	__asm__ __volatile__( "fninit" : : : "memory" );

	/* set FPU precision to 64-bit and round to nearest (as in QB) */
	_control87(PC_64|RC_NEAR, MCW_PC|MCW_RC);

	/* turn off blink */
	intensevideo();

	memset( &__fb_con, 0, sizeof( FB_CONSOLE_CTX ) );

	__fb_startup_cwd = getcwd(NULL, 1024);
	fb_hConvertPath( __fb_startup_cwd );

#if defined(FB_DOS_WATT32) && !defined(DISABLE_TCP)
	fb_DosTcpInitClock();
#endif

}

void fb_hEnd( int unused )
{
	(void)unused;

}

/* end of hinit.c */
