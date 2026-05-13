/* condition variable functions */

#include "../fb.h"
#include "../fb_private_thread.h"

FBCALL FBCOND *fb_CondCreate( void )
{
	return NULL;
}

FBCALL void fb_CondDestroy( FBCOND *cond )
{
	(void)cond;
}

FBCALL void fb_CondSignal( FBCOND *cond )
{
	(void)cond;
}

FBCALL void fb_CondBroadcast( FBCOND *cond )
{
	(void)cond;
}

FBCALL void fb_CondWait( FBCOND *cond, FBMUTEX *mutex )
{
	(void)cond;
	(void)mutex;
}
