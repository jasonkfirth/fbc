/* FreeBASIC DOS provider investigation: watt32-init.c
 *
 * Check the Watt-32 initialization convention used by the opt-in TCP device.
 * Run without a packet driver: initialization must return an error instead
 * of terminating the process. This does not test TCP traffic or rtlib I/O.
 */

#include <stdio.h>
#include <tcp.h>

/* The rtlib adapter currently depends on this Watt-32 internal switch. */
extern int _watt_do_exit;

int main( void )
{
	int old_do_exit = _watt_do_exit;
	int status;

	_watt_do_exit = 0;
	status = sock_init();
	_watt_do_exit = old_do_exit;
	if( status == 0 ) {
		sock_exit();
		puts( "FAIL precondition: run without a packet driver" );
		return 1;
	}
	printf( "PASS initialization returned error %d: %s\n", status,
	        sock_init_err( status ) );
	sock_exit();
	return 0;
}

/* end of watt32-init.c */
