/* FreeBASIC DOS provider investigation: pdmlwp-preempt.c
 *
 * Check PDMLWP timer preemption using shared-memory workers with no library
 * calls in their handoff loops. This probe does not establish libc, rtlib,
 * graphics, TLS, or SMP compatibility. Run in a disposable guest with an
 * external timeout because a cooperative provider will hang deliberately.
 */

#include <stdio.h>
#include <string.h>
#include <pc.h>
#include <lwp.h>

_Static_assert( __atomic_always_lock_free( sizeof( unsigned int ), 0 ),
                "The handoff must not call an atomic support library" );

static unsigned int turn;
static unsigned int finished[2];
static unsigned int identity[2] = { 0, 1 };

static void handoff_worker( void *argument )
{
	unsigned int id = *(unsigned int *)argument;
	unsigned int round;

	for( round = 0; round < 8; round++ ) {
		while( __atomic_load_n( &turn, __ATOMIC_ACQUIRE ) != id ) {
		}
		__atomic_store_n( &turn, 1 - id, __ATOMIC_RELEASE );
	}
	__atomic_store_n( &finished[id], 1, __ATOMIC_RELEASE );
}

int main( int argc, char **argv )
{
	FILE *log;
	unsigned int i;
	int worker;
	int use_pit = argc == 2 && strcmp( argv[1], "pit" ) == 0;

	if( argc > 2 || (argc == 2 && !use_pit && strcmp( argv[1], "rtc" ) != 0) ) {
		puts( "usage: pdmlwp rtc|pit" );
		return 1;
	}
	/* RTC64 leaves the BIOS PIT rate unchanged. The alternate IRQ0 experiment
	 * uses 1000 Hz, which is the unit expected by PDMLWP's millisecond clock.
	 */
	if( !lwp_init( use_pit ? 0 : 8, use_pit ? 1000 : RTC64 ) ) {
		puts( "FAIL lwp_init" );
		return 1;
	}
	lwp_thread_disable();
	for( i = 0; i < 2; i++ ) {
		/* 64 KiB permits scheduler and signal frames as well as worker data. */
		worker = lwp_spawn( handoff_worker, &identity[i], 65536, 1 );
		if( worker < 0 ) {
			puts( "FAIL lwp_spawn" );
			return 1;
		}
	}
	log = fopen( "PHASE.TXT", "w" );
	if( log == NULL )
		return 1;
	/* PIC mask bits reveal whether the selected BIOS routes IRQ8 at all. */
	if( fprintf( log, "workers created; enabling %s scheduler; PIC=%02x/%02x\n",
	             use_pit ? "PIT" : "RTC", inportb( 0x21 ), inportb( 0xa1 ) ) < 0 ) {
		fclose( log );
		return 1;
	}
	if( fclose( log ) != 0 )
		return 1;
	lwp_thread_enable();
	while( !__atomic_load_n( &finished[0], __ATOMIC_ACQUIRE ) ||
	       !__atomic_load_n( &finished[1], __ATOMIC_ACQUIRE ) ) {
	}
	/* The main thread and PDMLWP's internal cleanup thread remain alive. */
	while( lwp_getactive() != 2 ) {
	}
	lwp_thread_disable();
	puts( "PASS preemptive handoff" );
	return 0;
}

/* end of pdmlwp-preempt.c */
