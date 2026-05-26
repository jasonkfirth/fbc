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

#if defined(_WIN32) || defined(HOST_XBOX)

#include <windows.h>

static CRITICAL_SECTION g_fb_sfx_runtime_lock;
static CRITICAL_SECTION g_fb_sfx_driver_io_lock;
static volatile LONG g_fb_sfx_runtime_lock_ready = 0;
static volatile DWORD g_fb_sfx_runtime_lock_owner = 0;
static volatile DWORD g_fb_sfx_driver_io_lock_owner = 0;
static int g_fb_sfx_runtime_lock_depth = 0;
static int g_fb_sfx_driver_io_lock_depth = 0;

static void fb_sfxRuntimeEnsureLock(void)
{
    LONG state;

    state = InterlockedCompareExchange(&g_fb_sfx_runtime_lock_ready, 1, 0);
    if (state == 0)
    {
        InitializeCriticalSection(&g_fb_sfx_runtime_lock);
        InitializeCriticalSection(&g_fb_sfx_driver_io_lock);
        InterlockedExchange(&g_fb_sfx_runtime_lock_ready, 2);
        return;
    }

    while (InterlockedCompareExchange(&g_fb_sfx_runtime_lock_ready, 2, 2) != 2)
        Sleep(0);
}

static void fb_sfxEnterRecursiveCriticalSection(CRITICAL_SECTION *lock,
                                                volatile DWORD *owner,
                                                int *depth)
{
    DWORD thread_id;

    thread_id = GetCurrentThreadId();
    if (*owner == thread_id)
    {
        ++(*depth);
        return;
    }

    EnterCriticalSection(lock);
    *owner = thread_id;
    *depth = 1;
}

static void fb_sfxLeaveRecursiveCriticalSection(CRITICAL_SECTION *lock,
                                                volatile DWORD *owner,
                                                int *depth)
{
    DWORD thread_id;

    thread_id = GetCurrentThreadId();
    if (*owner != thread_id || *depth <= 0)
        return;

    --(*depth);
    if (*depth > 0)
        return;

    *owner = 0;
    LeaveCriticalSection(lock);
}

#elif defined(HOST_WII)

#include <ogc/mutex.h>

static mutex_t g_fb_sfx_runtime_lock = LWP_MUTEX_NULL;
static mutex_t g_fb_sfx_driver_io_lock = LWP_MUTEX_NULL;
static int g_fb_sfx_runtime_lock_ready = 0;

static void fb_sfxRuntimeEnsureLock(void)
{
    mutex_t runtime_lock;
    mutex_t driver_io_lock;

    if (g_fb_sfx_runtime_lock_ready)
        return;

    /*
        libogc mutexes can be recursive.  sfxlib relies on that behavior
        because command handlers may call helpers that also take the runtime
        lock while preserving the same high-level operation.

        The first lock initialization happens before the Wii audio worker is
        started, so a simple readiness flag is enough here.  If either mutex
        cannot be created, leave the handles null and let the lock/unlock
        wrappers behave as no-ops instead of crashing during startup.
    */

    runtime_lock = LWP_MUTEX_NULL;
    driver_io_lock = LWP_MUTEX_NULL;

    if (LWP_MutexInit(&runtime_lock, 1) != 0)
        return;

    if (LWP_MutexInit(&driver_io_lock, 1) != 0)
    {
        LWP_MutexDestroy(runtime_lock);
        return;
    }

    g_fb_sfx_runtime_lock = runtime_lock;
    g_fb_sfx_driver_io_lock = driver_io_lock;
    g_fb_sfx_runtime_lock_ready = 1;
}

#else

#include <pthread.h>

static pthread_mutex_t g_fb_sfx_runtime_lock;
static pthread_mutex_t g_fb_sfx_driver_io_lock;
static pthread_once_t g_fb_sfx_runtime_lock_once = PTHREAD_ONCE_INIT;

static void fb_sfxRuntimeLockCreate(void)
{
    pthread_mutexattr_t attr;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&g_fb_sfx_runtime_lock, &attr);
    pthread_mutex_init(&g_fb_sfx_driver_io_lock, &attr);
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

#if defined(_WIN32) || defined(HOST_XBOX)
    fb_sfxEnterRecursiveCriticalSection(&g_fb_sfx_runtime_lock,
                                        &g_fb_sfx_runtime_lock_owner,
                                        &g_fb_sfx_runtime_lock_depth);
#elif defined(HOST_WII)
    if (g_fb_sfx_runtime_lock != LWP_MUTEX_NULL)
        LWP_MutexLock(g_fb_sfx_runtime_lock);
#else
    pthread_mutex_lock(&g_fb_sfx_runtime_lock);
#endif
}

void fb_sfxRuntimeUnlock(void)
{
#if defined(_WIN32) || defined(HOST_XBOX)
    if (InterlockedCompareExchange(&g_fb_sfx_runtime_lock_ready, 2, 2) == 2)
        fb_sfxLeaveRecursiveCriticalSection(&g_fb_sfx_runtime_lock,
                                            &g_fb_sfx_runtime_lock_owner,
                                            &g_fb_sfx_runtime_lock_depth);
#elif defined(HOST_WII)
    if (g_fb_sfx_runtime_lock != LWP_MUTEX_NULL)
        LWP_MutexUnlock(g_fb_sfx_runtime_lock);
#else
    pthread_mutex_unlock(&g_fb_sfx_runtime_lock);
#endif
}

void fb_sfxDriverIoLock(void)
{
    fb_sfxRuntimeEnsureLock();

#if defined(_WIN32) || defined(HOST_XBOX)
    fb_sfxEnterRecursiveCriticalSection(&g_fb_sfx_driver_io_lock,
                                        &g_fb_sfx_driver_io_lock_owner,
                                        &g_fb_sfx_driver_io_lock_depth);
#elif defined(HOST_WII)
    if (g_fb_sfx_driver_io_lock != LWP_MUTEX_NULL)
        LWP_MutexLock(g_fb_sfx_driver_io_lock);
#else
    pthread_mutex_lock(&g_fb_sfx_driver_io_lock);
#endif
}

void fb_sfxDriverIoUnlock(void)
{
#if defined(_WIN32) || defined(HOST_XBOX)
    if (InterlockedCompareExchange(&g_fb_sfx_runtime_lock_ready, 2, 2) == 2)
        fb_sfxLeaveRecursiveCriticalSection(&g_fb_sfx_driver_io_lock,
                                            &g_fb_sfx_driver_io_lock_owner,
                                            &g_fb_sfx_driver_io_lock_depth);
#elif defined(HOST_WII)
    if (g_fb_sfx_driver_io_lock != LWP_MUTEX_NULL)
        LWP_MutexUnlock(g_fb_sfx_driver_io_lock);
#else
    pthread_mutex_unlock(&g_fb_sfx_driver_io_lock);
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

void fb_sfxDriverIoLock(void)
{
}

void fb_sfxDriverIoUnlock(void)
{
}

#endif
