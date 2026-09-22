/* FreeBASIC DOS provider tests: rtlib-threads.c
 *
 * Exercise real runtime threads under a watchdog: involuntary handoffs, x87
 * register/control preservation, errno and rtlib TLS, heap contention, mutex
 * ownership, condition broadcast, join and detach. No emulator APIs or timing
 * shortcuts participate in the worker handoff.
 */

#include "../../src/rtlib/fb.h"
#include "../../src/rtlib/dos/fb_dos_thread.h"
#include "../../src/sfxlib/fb_sfx.h"
#include "../../src/sfxlib/fb_sfx_internal.h"
#include "../../src/sfxlib/dos/fb_sfx_msdos.h"
#include <errno.h>
#include <float.h>

static unsigned int turn;
static unsigned int completed[2];
static unsigned int identities[2] = { 0, 1 };
static unsigned int failed;
static FBMUTEX *mutex;
static FBCOND *condition;
static int waiting;
static int released;
static int counter;
static unsigned int detached_done;

static int sleep_across_wrap( void )
{
	extern volatile unsigned long _lwp_ticks;
	clock_t started;
	double elapsed;
	int previous = fb_DosThreadEnter();
	/* No application sleepers remain here. Move only the scheduler's clock
	 * to its wrap boundary; the independent BIOS clock measures the wait.
	 */
	_lwp_ticks = ULONG_MAX - 2;
	fb_DosThreadLeave( previous );
	started = clock();
	fb_Delay( 160 );
	elapsed = (double)(clock() - started) / CLOCKS_PER_SEC;
	/* Allow one 55 ms BIOS tick of measurement granularity. */
	if( elapsed < 0.1 || elapsed > 3.0 || _lwp_ticks > 64 ) {
		puts( "FAIL sleep across RTC clock wrap" );
		return 0;
	}
	puts( "PASS sleep across RTC clock wrap" );
	return 1;
}

static void handoff( void *argument )
{
	unsigned int id = *(unsigned int *)argument, round;
	unsigned short control, observed;
	double original = id ? 123.5 : 0.25, actual;
	FBTHREAD *self = fb_ThreadSelf();
	fb_ErrorSetNum( 100 + id );
	control = id ? 0x077F : 0x0B7F; /* x87 extended precision, down/up rounding */
	__asm__ __volatile__( "fldcw %0" : : "m"(control) );
	errno = 200 + id;
	__asm__ __volatile__( "fldl %0" : : "m"(original) : "st" );
	for( round = 0; round < 16; round++ ) {
		while( __atomic_load_n( &turn, __ATOMIC_ACQUIRE ) != id ) {
		}
		__atomic_store_n( &completed[id], round + 1, __ATOMIC_RELEASE );
		__atomic_store_n( &turn, 1 - id, __ATOMIC_RELEASE );
	}
	__asm__ __volatile__( "fstpl %0; fnstcw %1" : "=m"(actual), "=m"(observed) : : "st" );
	if( actual != original || observed != control || errno != 200 + (int)id ||
	    fb_ErrorGetNum() != 100 + (int)id || fb_ThreadSelf() != self )
		__atomic_store_n( &failed, 1, __ATOMIC_RELEASE );
}

static void contended( void *unused )
{
	int i;
	(void)unused;
	fb_MutexLock( mutex );
	waiting++;
	while( !released )
		fb_CondWait( condition, mutex );
	fb_MutexUnlock( mutex );
	for( i = 0; i < 2000; i++ ) {
		unsigned char *grown;
		unsigned char *block = malloc( 1000 + i );
		if( !block ) {
			__atomic_store_n( &failed, 1, __ATOMIC_RELEASE );
			return;
		}
		memset( block, 0xA5, 1000 + i );
		grown = realloc( block, 3000 + i );
		if( !grown ) {
			__atomic_store_n( &failed, 1, __ATOMIC_RELEASE );
			free( block );
			return;
		}
		block = grown;
		if( block[999 + i] != 0xA5 )
			__atomic_store_n( &failed, 1, __ATOMIC_RELEASE );
		free( block );
		fb_MutexLock( mutex );
		counter++;
		fb_MutexUnlock( mutex );
	}
}

static void detached( void *unused )
{
	(void)unused;
	fb_Delay( 20 );
	__atomic_store_n( &detached_done, 1, __ATOMIC_RELEASE );
}

int main( int argc, char **argv )
{
	FBTHREAD *workers[2], *worker;
	FBTHREAD *main_thread;
	int i, ready;
	unsigned short control, observed;
	setvbuf( stdout, NULL, _IONBF, 0 );
	fb_hRtInit();
	if( argc > 1 && strcmp( argv[1], "speaker" ) == 0 ) {
		/* Run the same context/heap tests with the fast RTC consumer active.
		 * Any x87 or selector damage from the IRQ must still fail the test.
		 */
		if( fb_sfxInit() != 0 || !fb_sfxMsdosPcSpeakerIrqActive() ) {
			puts( "FAIL speaker interrupt initialization" );
			return 1;
		}
		fb_sfxSoundChannel( 0, 440, 30.0f, 0.5f );
	}
	main_thread = fb_ThreadSelf();
	fb_ErrorSetNum( 333 );
	__asm__ __volatile__( "fnstcw %0" : "=m"(control) );
	if( fb_ThreadCreate( NULL, NULL, 0 ) || fb_ThreadCreate( detached, NULL, -1 ) )
		return 1;
	for( i = 0; i < 2; i++ ) {
		workers[i] = fb_ThreadCreate( handoff, &identities[i], 0 );
		if( !workers[i] ) {
			puts( "FAIL thread creation" );
			return 1;
		}
	}
	for( i = 0; i < 2; i++ )
		fb_ThreadWait( workers[i] );
	__asm__ __volatile__( "fnstcw %0" : "=m"(observed) );
	if( completed[0] != 16 || completed[1] != 16 || failed ||
	    observed != control || fb_ErrorGetNum() != 333 || fb_ThreadSelf() != main_thread ) {
		puts( "FAIL preemption, FPU, errno or TLS" );
		return 1;
	}
	puts( "PASS preemption, x87, errno and TLS" );
	mutex = fb_MutexCreate();
	condition = fb_CondCreate();
	if( !mutex || !condition )
		return 1;
	for( i = 0; i < 2; i++ ) {
		workers[i] = fb_ThreadCreate( contended, NULL, 0 );
		if( !workers[i] )
			return 1;
	}
	do {
		fb_MutexLock( mutex );
		ready = waiting == 2;
		fb_MutexUnlock( mutex );
		fb_Delay( 1 );
	} while( !ready );
	fb_MutexLock( mutex );
	released = 1;
	fb_CondBroadcast( condition );
	fb_MutexUnlock( mutex );
	for( i = 0; i < 2; i++ )
		fb_ThreadWait( workers[i] );
	fb_CondDestroy( condition );
	fb_MutexDestroy( mutex );
	if( counter != 4000 || failed ) {
		puts( "FAIL mutex, condition or heap" );
		return 1;
	}
	worker = fb_ThreadCreate( detached, NULL, 0 );
	if( !worker )
		return 1;
	fb_ThreadDetach( worker );
	while( !__atomic_load_n( &detached_done, __ATOMIC_ACQUIRE ) )
		fb_Delay( 1 );
	fb_Delay( 20 );
	puts( "PASS mutex, condition broadcast, allocation, join and detach" );
	/* Do not move the scheduler clock underneath the optional audio worker. */
	if( argc == 1 && !sleep_across_wrap() )
		return 1;
	fb_hRtExit();
	return 0;
}

/* end of rtlib-threads.c */
