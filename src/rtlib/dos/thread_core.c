/* thread creation and destruction functions */

#include "../fb.h"
#include "../fb_private_thread.h"

FBCALL FBTHREAD *fb_ThreadCreate( FB_THREADPROC proc, void *param, ssize_t stack_size )
{
	(void)proc;
	(void)param;
	(void)stack_size;
	return NULL;
}

FBCALL void fb_ThreadWait( FBTHREAD *thread )
{
	(void)thread;
}
