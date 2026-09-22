/* FreeBASIC DOS provider tests: sfx-background.c
 *
 * Require Sound Blaster playback to advance while the application spins
 * without calling the sound pump or a scheduling API. Save the accepted PCM
 * for host analysis, then exercise foreground sound and repeated shutdown.
 * This does not substitute a null driver for hardware playback.
 */

#include "../../src/rtlib/fb.h"
#include "../../src/rtlib/fb_private_thread.h"
#include "../../src/sfxlib/fb_sfx.h"
#include "../../src/sfxlib/fb_sfx_internal.h"
#include "../../src/sfxlib/fb_sfx_driver.h"
#include "../../src/sfxlib/dos/fb_sfx_msdos.h"
#include <time.h>
#include <pc.h>

extern volatile int _lwp_interrupt_pending;

static void phase( const char *name )
{
	int previous = fb_DosThreadEnter();
	lwp *thread = _lwp_cur;
	fprintf( stderr, "%s: scheduler disabled=%d\n", name, previous );
	fprintf( stderr, "  pending=%d masks=%02x/%02x\n", _lwp_interrupt_pending,
	         inportb(0x21), inportb(0xA1) );
	if( thread ) {
		do {
			fprintf( stderr, "  id=%d state=%d\n", thread->lwpid, thread->status );
			thread = thread->next;
		} while( thread != _lwp_cur );
	}
	fflush( stderr );
	fb_DosThreadLeave( previous );
}

int main( int argc, char **argv )
{
	uclock_t start;
	volatile unsigned long iterations = 0;
	int cycle;
	setvbuf( stdout, NULL, _IONBF, 0 );
	if( !freopen( "ERROR.TXT", "w", stderr ) )
		return 1;
	fb_hRtInit();
	if( argc > 1 && strcmp( argv[1], "fallback" ) == 0 ) {
		unsigned char rtc_b;
		outportb( 0x70, 0x0B );
		rtc_b = inportb( 0x71 );
		/* Simulate an existing periodic RTC owner. The provider must refuse
		 * to take it over, and sound must continue through the idle hook.
		 */
		outportb( 0x70, 0x0B );
		outportb( 0x71, rtc_b | 0x40 );
		if( fb_sfxInit() != 0 || fb_sfxMsdosWorkerActive() ||
		    strcmp( __fb_sfx->driver->name, "SoundBlaster" ) != 0 )
			return 1;
		fb_sfxSoundChannel( 0, 440, 0.3f, 0.5f );
		fb_Delay( 250 );
		if( fb_sfxMsdosIrqCount() == 0 )
			return 1;
		fb_sfxExit();
		outportb( 0x70, 0x0B );
		outportb( 0x71, rtc_b );
		puts( "PASS unavailable scheduler retains foreground audio" );
		fb_hRtExit();
		return 0;
	}
	for( cycle = 0; cycle < 3; cycle++ ) {
		if( fb_sfxInit() != 0 || !__fb_sfx || !__fb_sfx->driver ||
		    strcmp( __fb_sfx->driver->name, "SoundBlaster" ) != 0 ||
		    !fb_sfxMsdosWorkerActive() ) {
			puts( "FAIL Sound Blaster worker initialization" );
			return 1;
		}
		if( fb_sfxOutputCaptureStart() != 0 ) {
			puts( "FAIL output capture initialization" );
			return 1;
		}
		fb_sfxSoundChannel( 0, 440, 2.0f, 0.5f );
		phase( "before spin" );
		start = uclock();
		while( uclock() - start < UCLOCKS_PER_SEC )
			iterations++;
		phase( "after spin" );
		fb_sfxOutputCaptureStop();
		if( fb_sfxOutputCaptureSave( cycle == 0 ? "AUDIO.WAV" : "REPEAT.WAV" ) != 0 ||
		    strcmp( __fb_sfx->driver->name, "SoundBlaster" ) != 0 ) {
			puts( "FAIL background PCM or driver fallback" );
			return 1;
		}
		/* An IRQ was consumed, rather than just sleeping for an estimated block. */
		if( fb_sfxMsdosIrqCount() == 0 ) {
			puts( "FAIL Sound Blaster completion interrupt" );
			return 1;
		}
		fb_sfxSoundLegacy2( 880, 2 );
		fb_sfxExit();
		printf( "PASS audio cycle %d\n", cycle + 1 );
	}
	if( !iterations )
		return 1;
	printf( "PASS background audio with %lu foreground iterations\n", iterations );
	fb_hRtExit();
	return 0;
}

/* end of sfx-background.c */
