/* FreeBASIC DOS provider investigation: fsu-preempt.c
 *
 * Check FSU Pthreads startup, joins, and scheduling independently of rtlib.
 * The handoff workers deliberately make no library or scheduler calls. A
 * cooperative scheduler cannot complete their shared-memory handshake.
 * This is a provider probe, not an implementation of DOS -mt or an SMP test.
 * Run each mode in a separate guest with an external timeout.
 */

#include <stdio.h>
#include <string.h>
#include <pthread.h>

/* GCC emits inline atomic instructions for aligned 32-bit words on DJGPP. */
_Static_assert( __atomic_always_lock_free( sizeof( unsigned int ), 0 ),
                "The handoff must not call an atomic support library" );

static unsigned int turn;
static unsigned int completed[2];
static unsigned int identity[2] = { 0, 1 };

static int phase( const char *message )
{
	/* DOS may not update directory sizes until close. Preserve the last
	 * setup phase even when the external watchdog stops a hung guest.
	 */
	FILE *log = fopen( "PHASE.TXT", "a" );
	int failed;
	if( log == NULL )
		return 1;
	failed = fprintf( log, "%s\n", message ) < 0;
	return fclose( log ) != 0 || failed;
}

static void *identity_worker( void *argument )
{
	return argument;
}

static void *handoff_worker( void *argument )
{
	unsigned int id = *(unsigned int *)argument;
	unsigned int round;

	/* Eight handoffs require repeated involuntary switches, not just startup. */
	for( round = 0; round < 8; round++ ) {
		while( __atomic_load_n( &turn, __ATOMIC_ACQUIRE ) != id ) {
		}
		__atomic_store_n( &completed[id], round + 1, __ATOMIC_RELEASE );
		__atomic_store_n( &turn, 1 - id, __ATOMIC_RELEASE );
	}
	return argument;
}

int main( int argc, char **argv )
{
	pthread_attr_t attributes;
	pthread_t workers[2];
	void *result;
	unsigned int i;
	int policy, status;
	int handoff;

	if( argc != 2 || (strcmp( argv[1], "join" ) != 0 &&
	                 strcmp( argv[1], "fifo" ) != 0 &&
	                 strcmp( argv[1], "rr" ) != 0) ) {
		puts( "usage: fsuprobe join|fifo|rr" );
		return 1;
	}

	handoff = strcmp( argv[1], "join" ) != 0;
	policy = strcmp( argv[1], "rr" ) == 0 ? SCHED_RR : SCHED_FIFO;
	if( phase( "before pthread_init" ) )
		return 1;
	pthread_init();
	if( phase( "after pthread_init" ) )
		return 1;
	status = pthread_attr_init( &attributes );
	if( status != 0 ) {
		printf( "FAIL attr_init: %d\n", status );
		return 1;
	}
	status = pthread_attr_setschedpolicy( &attributes, policy );
	if( status != 0 ) {
		printf( "UNSUPPORTED %s policy: %d\n", argv[1], status );
		pthread_attr_destroy( &attributes );
		return 2;
	}

	printf( "START %s\n", argv[1] );
	fflush( stdout );
	if( phase( "before creating workers" ) )
		return 1;
	for( i = 0; i < 2; i++ ) {
		status = pthread_create( &workers[i], &attributes,
		                        handoff ? handoff_worker : identity_worker,
		                        &identity[i] );
		if( status != 0 ) {
			/* A failed setup ends the disposable guest process. No peer may
			 * be joined here: it could be waiting for the missing worker.
			 */
			printf( "FAIL create[%u]: %d\n", i, status );
			return 1;
		}
	}
	pthread_attr_destroy( &attributes );
	for( i = 0; i < 2; i++ ) {
		status = pthread_join( workers[i], &result );
		if( status != 0 || result != &identity[i] ) {
			printf( "FAIL join[%u]: %d\n", i, status );
			return 1;
		}
	}
	if( handoff && (__atomic_load_n( &completed[0], __ATOMIC_ACQUIRE ) != 8 ||
	                __atomic_load_n( &completed[1], __ATOMIC_ACQUIRE ) != 8) ) {
		puts( "FAIL incomplete handoff" );
		return 1;
	}
	printf( "PASS %s\n", argv[1] );
	return 0;
}

/* end of fsu-preempt.c */
