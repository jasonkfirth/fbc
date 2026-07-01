/*
    FreeBASIC runtime library
    -------------------------

    File: fb_nuttx_thread.c

    Purpose:

        Provide threading primitives for the small NuttX runtime.

    Responsibilities:

        - wrap pthread_create()
        - wrap pthread mutexes and condition variables
        - preserve the BASIC thread procedure signature
        - release thread bookkeeping after THREADWAIT

    This file intentionally does NOT contain:

        - thread-local runtime contexts
        - thread cancellation
*/

#include "fb.h"

#include <pthread.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>

/* ------------------------------------------------------------------------- */
/* Thread handle                                                             */
/* ------------------------------------------------------------------------- */

typedef void (*FB_NUTTX_THREAD_PROC)(void *);

#define FB_NUTTX_THREADCALL_MAX_ARGS 8

enum {
    FB_THREADCALL_STDCALL,
    FB_THREADCALL_CDECL,
    FB_THREADCALL_INT8,
    FB_THREADCALL_UINT8,
    FB_THREADCALL_INT16,
    FB_THREADCALL_UINT16,
    FB_THREADCALL_INT32,
    FB_THREADCALL_UINT32,
    FB_THREADCALL_INT64,
    FB_THREADCALL_UINT64,
    FB_THREADCALL_FLOAT32,
    FB_THREADCALL_FLOAT64,
    FB_THREADCALL_STRUCT,
    FB_THREADCALL_PTR
};

typedef struct FB_NUTTX_THREAD {
    pthread_t id;
    pthread_mutex_t lock;
    int heap_allocated;
    int detached;
    int exited;
    struct FB_NUTTX_THREAD *next;
} FB_NUTTX_THREAD;

typedef struct FB_NUTTX_MUTEX {
    pthread_mutex_t mutex;
} FB_NUTTX_MUTEX;

typedef struct FB_NUTTX_COND {
    pthread_cond_t cond;
} FB_NUTTX_COND;

typedef struct FB_NUTTX_THREAD_START {
    FB_NUTTX_THREAD *thread;
    FB_NUTTX_THREAD_PROC proc;
    void *param;
} FB_NUTTX_THREAD_START;

typedef struct FB_NUTTX_THREADCALL {
    void *proc;
    int32 arg_count;
    uintptr_t args[FB_NUTTX_THREADCALL_MAX_ARGS];
} FB_NUTTX_THREADCALL;

static pthread_mutex_t fb_nuttx_thread_map_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t fb_nuttx_main_thread_lock = PTHREAD_MUTEX_INITIALIZER;
static FB_NUTTX_THREAD *fb_nuttx_thread_map;
static FB_NUTTX_THREAD fb_nuttx_main_thread;
static int fb_nuttx_main_thread_ready;

/* ------------------------------------------------------------------------- */
/* FreeBASIC thread identity                                                 */
/* ------------------------------------------------------------------------- */

/*
    ThreadSelf() returns the FreeBASIC thread handle, not the raw pthread id.

    The normal hosted runtime stores that handle in TLS.  The early NuttX
    loadable-module image exports the pthread primitives needed for creation,
    joins, and mutexes, but not the pthread TLS calls.  Keep a small live-thread
    map instead so BASIC programs see the same identity model without depending
    on kernel symbols that this target does not currently export.
*/

static void fb_nuttx_thread_map_add(FB_NUTTX_THREAD *thread)
{
    if (thread == NULL)
        return;

    pthread_mutex_lock(&fb_nuttx_thread_map_lock);

    thread->next = fb_nuttx_thread_map;
    fb_nuttx_thread_map = thread;

    pthread_mutex_unlock(&fb_nuttx_thread_map_lock);
}

static void fb_nuttx_thread_map_remove(FB_NUTTX_THREAD *thread)
{
    FB_NUTTX_THREAD **scan;

    if (thread == NULL)
        return;

    pthread_mutex_lock(&fb_nuttx_thread_map_lock);

    scan = &fb_nuttx_thread_map;

    while (*scan != NULL) {
        if (*scan == thread) {
            *scan = thread->next;
            thread->next = NULL;
            break;
        }

        scan = &(*scan)->next;
    }

    pthread_mutex_unlock(&fb_nuttx_thread_map_lock);
}

static FB_NUTTX_THREAD *fb_nuttx_thread_get_self(void)
{
    FB_NUTTX_THREAD *thread;
    pthread_t id;

    id = pthread_self();

    pthread_mutex_lock(&fb_nuttx_thread_map_lock);

    thread = fb_nuttx_thread_map;

    while (thread != NULL) {
        if (thread->id == id)
            break;

        thread = thread->next;
    }

    pthread_mutex_unlock(&fb_nuttx_thread_map_lock);

    return thread;
}

static FB_NUTTX_THREAD *fb_nuttx_thread_get_main(void)
{
    pthread_mutex_lock(&fb_nuttx_main_thread_lock);

    if (!fb_nuttx_main_thread_ready) {
        fb_nuttx_main_thread.id = pthread_self();
        fb_nuttx_main_thread.heap_allocated = 0;
        fb_nuttx_main_thread.detached = 0;
        fb_nuttx_main_thread.exited = 0;
        fb_nuttx_main_thread.next = NULL;
        fb_nuttx_main_thread_ready = 1;
    }

    pthread_mutex_unlock(&fb_nuttx_main_thread_lock);

    return &fb_nuttx_main_thread;
}

static void fb_nuttx_thread_free(FB_NUTTX_THREAD *thread)
{
    if ((thread == NULL) || !thread->heap_allocated)
        return;

    pthread_mutex_destroy(&thread->lock);
    free(thread);
}

static void fb_nuttx_thread_mark_exited(FB_NUTTX_THREAD *thread)
{
    int should_free;

    if ((thread == NULL) || !thread->heap_allocated)
        return;

    should_free = 0;

    pthread_mutex_lock(&thread->lock);

    thread->exited = 1;
    if (thread->detached)
        should_free = 1;

    pthread_mutex_unlock(&thread->lock);

    if (should_free)
        fb_nuttx_thread_free(thread);
}

/* ------------------------------------------------------------------------- */
/* pthread trampoline                                                        */
/* ------------------------------------------------------------------------- */

static void *fb_nuttx_thread_start(void *arg)
{
    FB_NUTTX_THREAD_START *start;
    FB_NUTTX_THREAD *thread;
    FB_NUTTX_THREAD_PROC proc;
    void *param;

    start = (FB_NUTTX_THREAD_START *)arg;

    if (start != NULL) {
        thread = start->thread;
        proc = start->proc;
        param = start->param;

        free(start);

        if (thread != NULL) {
            thread->id = pthread_self();
            fb_nuttx_thread_map_add(thread);
        }

        if (proc != NULL)
            proc(param);

        fb_nuttx_thread_map_remove(thread);
        fb_nuttx_thread_mark_exited(thread);
    }

    return NULL;
}

static void fb_nuttx_threadcall_start(void *arg)
{
    FB_NUTTX_THREADCALL *call;

    call = (FB_NUTTX_THREADCALL *)arg;

    if (call == NULL)
        return;

    switch (call->arg_count) {
    case 0:
        ((void (*)(void))call->proc)();
        break;
    case 1:
        ((void (*)(uintptr_t))call->proc)(call->args[0]);
        break;
    case 2:
        ((void (*)(uintptr_t, uintptr_t))call->proc)(call->args[0],
            call->args[1]);
        break;
    case 3:
        ((void (*)(uintptr_t, uintptr_t, uintptr_t))call->proc)(
            call->args[0], call->args[1], call->args[2]);
        break;
    case 4:
        ((void (*)(uintptr_t, uintptr_t, uintptr_t, uintptr_t))call->proc)(
            call->args[0], call->args[1], call->args[2], call->args[3]);
        break;
    case 5:
        ((void (*)(uintptr_t, uintptr_t, uintptr_t, uintptr_t,
            uintptr_t))call->proc)(call->args[0], call->args[1],
            call->args[2], call->args[3], call->args[4]);
        break;
    case 6:
        ((void (*)(uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t,
            uintptr_t))call->proc)(call->args[0], call->args[1],
            call->args[2], call->args[3], call->args[4], call->args[5]);
        break;
    case 7:
        ((void (*)(uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t,
            uintptr_t, uintptr_t))call->proc)(call->args[0], call->args[1],
            call->args[2], call->args[3], call->args[4], call->args[5],
            call->args[6]);
        break;
    case 8:
        ((void (*)(uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t,
            uintptr_t, uintptr_t, uintptr_t))call->proc)(call->args[0],
            call->args[1], call->args[2], call->args[3], call->args[4],
            call->args[5], call->args[6], call->args[7]);
        break;
    default:
        break;
    }

    free(call);
}

static int fb_nuttx_threadcall_read_arg(uintptr_t *out_value,
    const int type_code, const void *value_ptr)
{
    if ((out_value == NULL) || (value_ptr == NULL))
        return 0;

    switch (type_code) {
    case FB_THREADCALL_INT8:
        *out_value = (uintptr_t)(intptr_t)*(const int8_t *)value_ptr;
        return 1;
    case FB_THREADCALL_UINT8:
        *out_value = (uintptr_t)*(const uint8_t *)value_ptr;
        return 1;
    case FB_THREADCALL_INT16:
        *out_value = (uintptr_t)(intptr_t)*(const int16_t *)value_ptr;
        return 1;
    case FB_THREADCALL_UINT16:
        *out_value = (uintptr_t)*(const uint16_t *)value_ptr;
        return 1;
    case FB_THREADCALL_INT32:
        *out_value = (uintptr_t)(intptr_t)*(const int32 *)value_ptr;
        return 1;
    case FB_THREADCALL_UINT32:
        *out_value = (uintptr_t)*(const uint32 *)value_ptr;
        return 1;
    case FB_THREADCALL_PTR:
        *out_value = (uintptr_t)*(void * const *)value_ptr;
        return 1;
    default:
        return 0;
    }
}

/* ------------------------------------------------------------------------- */
/* BASIC thread entry points                                                 */
/* ------------------------------------------------------------------------- */

void *fb_ThreadCreate(FB_NUTTX_THREAD_PROC proc, void *param,
    const int32 stack_size)
{
    FB_NUTTX_THREAD *thread;
    FB_NUTTX_THREAD_START *start;
    pthread_attr_t attr;
    int use_attr;

    if (proc == NULL)
        return NULL;

    thread = (FB_NUTTX_THREAD *)malloc(sizeof(FB_NUTTX_THREAD));
    if (thread == NULL)
        return NULL;

    if (pthread_mutex_init(&thread->lock, NULL) != 0) {
        free(thread);
        return NULL;
    }

    thread->heap_allocated = 1;
    thread->detached = 0;
    thread->exited = 0;
    thread->next = NULL;

    start = (FB_NUTTX_THREAD_START *)malloc(sizeof(FB_NUTTX_THREAD_START));
    if (start == NULL) {
        pthread_mutex_destroy(&thread->lock);
        free(thread);
        return NULL;
    }

    start->thread = thread;
    start->proc = proc;
    start->param = param;

    use_attr = 0;

    if (stack_size > 0) {
        if (pthread_attr_init(&attr) == 0) {
            if (pthread_attr_setstacksize(&attr, (size_t)stack_size) == 0)
                use_attr = 1;
            else
                pthread_attr_destroy(&attr);
        }
    }

    if (pthread_create(&thread->id, use_attr ? &attr : NULL,
                       fb_nuttx_thread_start, start) != 0) {
        if (use_attr)
            pthread_attr_destroy(&attr);

        free(start);
        pthread_mutex_destroy(&thread->lock);
        free(thread);
        return NULL;
    }

    if (use_attr)
        pthread_attr_destroy(&attr);

    return thread;
}

void fb_ThreadWait(void *thread_ptr)
{
    FB_NUTTX_THREAD *thread;
    int detached;

    if (thread_ptr == NULL)
        return;

    thread = (FB_NUTTX_THREAD *)thread_ptr;

    if (!thread->heap_allocated)
        return;

    if ((thread == fb_nuttx_thread_get_self()) ||
        (thread->id == pthread_self()))
        return;

    pthread_mutex_lock(&thread->lock);
    detached = thread->detached;
    pthread_mutex_unlock(&thread->lock);

    if (detached)
        return;

    pthread_join(thread->id, NULL);
    fb_nuttx_thread_free(thread);
}

void fb_ThreadDetach(void *thread_ptr)
{
    FB_NUTTX_THREAD *thread;
    int exited;

    if (thread_ptr == NULL)
        return;

    thread = (FB_NUTTX_THREAD *)thread_ptr;

    if (!thread->heap_allocated)
        return;

    pthread_mutex_lock(&thread->lock);
    if (thread->detached) {
        pthread_mutex_unlock(&thread->lock);
        return;
    }

    thread->detached = 1;
    exited = thread->exited;
    pthread_mutex_unlock(&thread->lock);

    pthread_detach(thread->id);

    if (exited)
        fb_nuttx_thread_free(thread);
}

void *fb_ThreadSelf(void)
{
    FB_NUTTX_THREAD *thread;

    thread = fb_nuttx_thread_get_self();
    if (thread != NULL)
        return thread;

    thread = fb_nuttx_thread_get_main();

    return thread;
}

void *fb_ThreadCall(void *proc, const int32 abi, const int32 stack_size,
    const int32 arg_count, ...)
{
    FB_NUTTX_THREADCALL *call;
    va_list ap;
    int32 i;

    /*
        The hosted runtime uses libffi so THREADCALL can invoke arbitrary
        procedure signatures.  The embedded NuttX seed runtime keeps a smaller
        path for word-sized scalar and pointer arguments, which covers the
        generated manual examples and many simple embedded programs on RV32.
    */
    if ((proc == NULL) || (arg_count < 0) ||
        (arg_count > FB_NUTTX_THREADCALL_MAX_ARGS))
        return NULL;

    if ((abi != FB_THREADCALL_CDECL) && (abi != FB_THREADCALL_STDCALL))
        return NULL;

    call = (FB_NUTTX_THREADCALL *)calloc(1, sizeof(FB_NUTTX_THREADCALL));

    if (call == NULL)
        return NULL;

    call->proc = proc;
    call->arg_count = arg_count;

    va_start(ap, arg_count);

    for (i = 0; i < arg_count; i++) {
        int type_code;
        void *value_ptr;

        type_code = va_arg(ap, int);
        value_ptr = va_arg(ap, void *);

        if (!fb_nuttx_threadcall_read_arg(&call->args[i], type_code,
                value_ptr)) {
            va_end(ap);
            free(call);
            return NULL;
        }
    }

    va_end(ap);

    return fb_ThreadCreate(fb_nuttx_threadcall_start, call, stack_size);
}

void *fb_MutexCreate(void)
{
    FB_NUTTX_MUTEX *mutex;

    mutex = (FB_NUTTX_MUTEX *)malloc(sizeof(FB_NUTTX_MUTEX));

    if (mutex == NULL)
        return NULL;

    if (pthread_mutex_init(&mutex->mutex, NULL) != 0) {
        free(mutex);
        return NULL;
    }

    return mutex;
}

void fb_MutexDestroy(void *mutex_ptr)
{
    FB_NUTTX_MUTEX *mutex;

    if (mutex_ptr == NULL)
        return;

    mutex = (FB_NUTTX_MUTEX *)mutex_ptr;

    pthread_mutex_destroy(&mutex->mutex);
    free(mutex);
}

void fb_MutexLock(void *mutex_ptr)
{
    FB_NUTTX_MUTEX *mutex;

    if (mutex_ptr == NULL)
        return;

    mutex = (FB_NUTTX_MUTEX *)mutex_ptr;
    pthread_mutex_lock(&mutex->mutex);
}

void fb_MutexUnlock(void *mutex_ptr)
{
    FB_NUTTX_MUTEX *mutex;

    if (mutex_ptr == NULL)
        return;

    mutex = (FB_NUTTX_MUTEX *)mutex_ptr;
    pthread_mutex_unlock(&mutex->mutex);
}

void *fb_CondCreate(void)
{
    FB_NUTTX_COND *cond;

    cond = (FB_NUTTX_COND *)malloc(sizeof(FB_NUTTX_COND));

    if (cond == NULL)
        return NULL;

    if (pthread_cond_init(&cond->cond, NULL) != 0) {
        free(cond);
        return NULL;
    }

    return cond;
}

void fb_CondDestroy(void *cond_ptr)
{
    FB_NUTTX_COND *cond;

    if (cond_ptr == NULL)
        return;

    cond = (FB_NUTTX_COND *)cond_ptr;

    pthread_cond_destroy(&cond->cond);
    free(cond);
}

void fb_CondSignal(void *cond_ptr)
{
    FB_NUTTX_COND *cond;

    if (cond_ptr == NULL)
        return;

    cond = (FB_NUTTX_COND *)cond_ptr;
    pthread_cond_signal(&cond->cond);
}

void fb_CondBroadcast(void *cond_ptr)
{
    FB_NUTTX_COND *cond;

    if (cond_ptr == NULL)
        return;

    cond = (FB_NUTTX_COND *)cond_ptr;
    pthread_cond_broadcast(&cond->cond);
}

void fb_CondWait(void *cond_ptr, void *mutex_ptr)
{
    FB_NUTTX_COND *cond;
    FB_NUTTX_MUTEX *mutex;

    if ((cond_ptr == NULL) || (mutex_ptr == NULL))
        return;

    cond = (FB_NUTTX_COND *)cond_ptr;
    mutex = (FB_NUTTX_MUTEX *)mutex_ptr;

    pthread_cond_wait(&cond->cond, &mutex->mutex);
}

/* end of fb_nuttx_thread.c */
