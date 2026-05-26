/*
    FreeBASIC runtime Wii mutex hooks
    ---------------------------------

    File: thread_mutex.c

    Purpose:

        Adapt FreeBASIC's runtime mutex abstraction to libogc LWP mutexes.

    Responsibilities:

        - create recursive mutexes for runtime and gfxlib locks
        - ignore NULL mutex handles safely
        - release libogc mutex handles before freeing wrapper storage

    This file intentionally does NOT contain:

        - thread creation
        - atomic flag helpers
        - scheduler policy
*/

#include "../fb.h"
#include "../fb_private_thread.h"

FBCALL FBMUTEX *fb_MutexCreate(void)
{
	FBMUTEX *mutex = (FBMUTEX *)malloc(sizeof(FBMUTEX));

	if (mutex == NULL)
		return NULL;

	mutex->id = LWP_MUTEX_NULL;
	if (LWP_MutexInit(&mutex->id, TRUE) < 0) {
		free(mutex);
		return NULL;
	}

	return mutex;
}

FBCALL void fb_MutexDestroy(FBMUTEX *mutex)
{
	if (mutex == NULL)
		return;

	if (mutex->id != LWP_MUTEX_NULL)
		LWP_MutexDestroy(mutex->id);

	free(mutex);
}

FBCALL void fb_MutexLock(FBMUTEX *mutex)
{
	if ((mutex != NULL) && (mutex->id != LWP_MUTEX_NULL))
		LWP_MutexLock(mutex->id);
}

FBCALL void fb_MutexUnlock(FBMUTEX *mutex)
{
	if ((mutex != NULL) && (mutex->id != LWP_MUTEX_NULL))
		LWP_MutexUnlock(mutex->id);
}

/* end of thread_mutex.c */
