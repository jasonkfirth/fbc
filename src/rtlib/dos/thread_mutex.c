/* mutex handling routines */

#include "../fb.h"
#include "../fb_private_thread.h"

FBCALL FBMUTEX *fb_MutexCreate( void )
{
	return NULL;
}

FBCALL void fb_MutexDestroy( FBMUTEX *mutex )
{
	(void)mutex;
}

FBCALL void fb_MutexLock( FBMUTEX *mutex )
{
	(void)mutex;
}

FBCALL void fb_MutexUnlock( FBMUTEX *mutex )
{
	(void)mutex;
}
