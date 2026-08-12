/*
    FreeBASIC runtime Wii condition variables
    -----------------------------------------

    File: thread_cond.c

    Purpose:

        Adapt FreeBASIC condition variables to libogc LWP conditions.

    Responsibilities:

        - allocate and initialize LWP condition handles
        - wait with the associated FreeBASIC mutex
        - signal, broadcast, destroy, and release condition handles safely

    This file intentionally does NOT contain:

        - mutex implementation
        - thread creation
        - scheduler policy
*/

#include "../fb.h"
#include "../fb_private_thread.h"
#include <ogc/cond.h>

struct _FBCOND {
	cond_t id;
};

FBCALL FBCOND *fb_CondCreate(void)
{
	FBCOND *cond = (FBCOND *)malloc(sizeof(FBCOND));

	if (cond == NULL)
		return NULL;

	cond->id = LWP_COND_NULL;
	if (LWP_CondInit(&cond->id) < 0) {
		free(cond);
		return NULL;
	}

	return cond;
}

FBCALL void fb_CondDestroy(FBCOND *cond)
{
	if (cond == NULL)
		return;

	if (cond->id != LWP_COND_NULL)
		LWP_CondDestroy(cond->id);

	free(cond);
}

FBCALL void fb_CondSignal(FBCOND *cond)
{
	if ((cond != NULL) && (cond->id != LWP_COND_NULL))
		LWP_CondSignal(cond->id);
}

FBCALL void fb_CondBroadcast(FBCOND *cond)
{
	if ((cond != NULL) && (cond->id != LWP_COND_NULL))
		LWP_CondBroadcast(cond->id);
}

FBCALL void fb_CondWait(FBCOND *cond, FBMUTEX *mutex)
{
	if ((cond != NULL) &&
	    (cond->id != LWP_COND_NULL) &&
	    (mutex != NULL) &&
	    (mutex->id != LWP_MUTEX_NULL)) {
		LWP_CondWait(cond->id, mutex->id);
	}
}

/* end of thread_cond.c */
