/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_thread.c

    Purpose:

        Provide a small internal locking layer for multithreaded builds.

        In single-threaded builds these helpers compile down to no-ops,
        so the ordinary runtime stays simple and fast.
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"

#if FB_SFX_MT_ENABLED

#if defined(_WIN32)

#include <windows.h>

static CRITICAL_SECTION g_fb_sfx_runtime_lock;
static volatile LONG g_fb_sfx_runtime_lock_ready = 0;

static void fb_sfxRuntimeEnsureLock(void)
{
    LONG state;

    state = InterlockedCompareExchange(&g_fb_sfx_runtime_lock_ready, 1, 0);
    if (state == 0)
    {
        InitializeCriticalSection(&g_fb_sfx_runtime_lock);
        InterlockedExchange(&g_fb_sfx_runtime_lock_ready, 2);
        return;
    }

    while (InterlockedCompareExchange(&g_fb_sfx_runtime_lock_ready, 2, 2) != 2)
        Sleep(0);
}

#else

#include <pthread.h>

static pthread_mutex_t g_fb_sfx_runtime_lock;
static pthread_once_t g_fb_sfx_runtime_lock_once = PTHREAD_ONCE_INIT;

static void fb_sfxRuntimeLockCreate(void)
{
    pthread_mutexattr_t attr;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&g_fb_sfx_runtime_lock, &attr);
    pthread_mutexattr_destroy(&attr);
}

static void fb_sfxRuntimeEnsureLock(void)
{
    pthread_once(&g_fb_sfx_runtime_lock_once, fb_sfxRuntimeLockCreate);
}

#endif

void fb_sfxRuntimeLockInit(void)
{
    fb_sfxRuntimeEnsureLock();
}

void fb_sfxRuntimeLockShutdown(void)
{
}

void fb_sfxRuntimeLock(void)
{
    fb_sfxRuntimeEnsureLock();

#if defined(_WIN32)
    EnterCriticalSection(&g_fb_sfx_runtime_lock);
#else
    pthread_mutex_lock(&g_fb_sfx_runtime_lock);
#endif
}

void fb_sfxRuntimeUnlock(void)
{
#if defined(_WIN32)
    if (InterlockedCompareExchange(&g_fb_sfx_runtime_lock_ready, 2, 2) == 2)
        LeaveCriticalSection(&g_fb_sfx_runtime_lock);
#else
    pthread_mutex_unlock(&g_fb_sfx_runtime_lock);
#endif
}

#else

void fb_sfxRuntimeLockInit(void)
{
}

void fb_sfxRuntimeLockShutdown(void)
{
}

void fb_sfxRuntimeLock(void)
{
}

void fb_sfxRuntimeUnlock(void)
{
}

#endif
