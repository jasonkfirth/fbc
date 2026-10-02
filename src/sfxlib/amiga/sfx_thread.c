/*
    FreeBASIC sound library for classic AmigaOS
    ------------------------------------------

    File: sfx_thread.c

    Purpose:
        Serialize mixer state and device writes through Exec semaphores.

    Responsibilities:
        - initialize the two recursive locks without an allocator dependency
        - keep driver waits separate from the lock protecting mixer state

    This file intentionally does NOT contain:
        - worker creation, device requests, or mixer operations

    Exec signal semaphores are recursive for their owning task. The common
    sound core drops the mixer lock before waiting on the driver lock, then
    drops it again during blocking I/O. A background worker can therefore
    finish its device request while the foreground task queues another tone.

    Forbid prevents task switching only during bounded lock initialization.
    No semaphore wait or device operation runs in that interval.
*/

#include "../fb_sfx_internal.h"

#include <exec/semaphores.h>
#include <proto/exec.h>

static struct SignalSemaphore runtime_lock;
static struct SignalSemaphore driver_lock;
static int locks_initialized;

void fb_sfxRuntimeLockInit(void)
{
    Forbid();
    if (!locks_initialized) {
        InitSemaphore(&runtime_lock);
        InitSemaphore(&driver_lock);
        locks_initialized = 1;
    }
    Permit();
}

void fb_sfxRuntimeLockShutdown(void) {}

void fb_sfxRuntimeLock(void)
{
    fb_sfxRuntimeLockInit();
    ObtainSemaphore(&runtime_lock);
}

void fb_sfxRuntimeUnlock(void) { ReleaseSemaphore(&runtime_lock); }

void fb_sfxDriverIoLock(void)
{
    fb_sfxRuntimeLockInit();
    if (runtime_lock.ss_Owner == FindTask(NULL))
        SFX_DEBUG("Amiga audio lock order: mixer nesting=%d", runtime_lock.ss_NestCount);
    ObtainSemaphore(&driver_lock);
}

void fb_sfxDriverIoUnlock(void) { ReleaseSemaphore(&driver_lock); }

/* end of sfx_thread.c */
