''
'' FreeBASIC Darwin pthread bindings
'' ---------------------------------
''
'' File: crt/darwin/pthread.bi
''
'' Purpose:
''
''     Expose the public POSIX thread interface provided by macOS libSystem.
''
'' Responsibilities:
''
''     - mirror Apple's opaque pthread object layouts and initializers
''     - declare the POSIX lifecycle, attribute, synchronization, and TLS APIs
''     - declare commonly used public Darwin thread and quality-of-service APIs
''
'' This file intentionally does NOT contain:
''
''     - glibc internals or Linux-only pthread extensions
''     - private Apple pthread SPI
''     - wrappers around FreeBASIC's native threading statements
''

#ifndef __crt_darwin_pthread_bi__
#define __crt_darwin_pthread_bi__

#include once "crt/long.bi"
#include once "crt/stddef.bi"
#include once "crt/sys/types.bi"
#include once "crt/sched.bi"
#include once "crt/time.bi"

const _PTHREAD_H = 1

'' -------------------------------------------------------------------------
'' Opaque object layouts
'' -------------------------------------------------------------------------

'' Apple includes the leading C long signature in addition to each published
'' __PTHREAD_*_SIZE__ byte count.  Keeping the fields opaque prevents callers
'' from depending on implementation data while preserving ABI size/alignment.
#ifdef __FB_64BIT__
	const __PTHREAD_SIZE__ = 8176
	const __PTHREAD_ATTR_SIZE__ = 56
	const __PTHREAD_MUTEXATTR_SIZE__ = 8
	const __PTHREAD_MUTEX_SIZE__ = 56
	const __PTHREAD_CONDATTR_SIZE__ = 8
	const __PTHREAD_COND_SIZE__ = 40
	const __PTHREAD_ONCE_SIZE__ = 8
	const __PTHREAD_RWLOCK_SIZE__ = 192
	const __PTHREAD_RWLOCKATTR_SIZE__ = 16
#else
	const __PTHREAD_SIZE__ = 4088
	const __PTHREAD_ATTR_SIZE__ = 36
	const __PTHREAD_MUTEXATTR_SIZE__ = 8
	const __PTHREAD_MUTEX_SIZE__ = 40
	const __PTHREAD_CONDATTR_SIZE__ = 4
	const __PTHREAD_COND_SIZE__ = 24
	const __PTHREAD_ONCE_SIZE__ = 4
	const __PTHREAD_RWLOCK_SIZE__ = 124
	const __PTHREAD_RWLOCKATTR_SIZE__ = 12
#endif

type __darwin_pthread_handler_rec
	__routine as sub cdecl(byval as any ptr)
	__arg as any ptr
	__next as __darwin_pthread_handler_rec ptr
end type

type _opaque_pthread_attr_t
	__sig as clong
	__opaque(0 to __PTHREAD_ATTR_SIZE__ - 1) as ubyte
end type
type pthread_attr_t as _opaque_pthread_attr_t

type _opaque_pthread_cond_t
	__sig as clong
	__opaque(0 to __PTHREAD_COND_SIZE__ - 1) as ubyte
end type
type pthread_cond_t as _opaque_pthread_cond_t

type _opaque_pthread_condattr_t
	__sig as clong
	__opaque(0 to __PTHREAD_CONDATTR_SIZE__ - 1) as ubyte
end type
type pthread_condattr_t as _opaque_pthread_condattr_t

type _opaque_pthread_mutex_t
	__sig as clong
	__opaque(0 to __PTHREAD_MUTEX_SIZE__ - 1) as ubyte
end type
type pthread_mutex_t as _opaque_pthread_mutex_t

type _opaque_pthread_mutexattr_t
	__sig as clong
	__opaque(0 to __PTHREAD_MUTEXATTR_SIZE__ - 1) as ubyte
end type
type pthread_mutexattr_t as _opaque_pthread_mutexattr_t

type _opaque_pthread_once_t
	__sig as clong
	__opaque(0 to __PTHREAD_ONCE_SIZE__ - 1) as ubyte
end type
type pthread_once_t as _opaque_pthread_once_t

type _opaque_pthread_rwlock_t
	__sig as clong
	__opaque(0 to __PTHREAD_RWLOCK_SIZE__ - 1) as ubyte
end type
type pthread_rwlock_t as _opaque_pthread_rwlock_t

type _opaque_pthread_rwlockattr_t
	__sig as clong
	__opaque(0 to __PTHREAD_RWLOCKATTR_SIZE__ - 1) as ubyte
end type
type pthread_rwlockattr_t as _opaque_pthread_rwlockattr_t

type _opaque_pthread_t
	__sig as clong
	__cleanup_stack as __darwin_pthread_handler_rec ptr
	__opaque(0 to __PTHREAD_SIZE__ - 1) as ubyte
end type

type pthread_t as _opaque_pthread_t ptr
type pthread_key_t as culong

'' -------------------------------------------------------------------------
'' Constants and static initializers
'' -------------------------------------------------------------------------

const PTHREAD_CREATE_JOINABLE = 1
const PTHREAD_CREATE_DETACHED = 2
const PTHREAD_INHERIT_SCHED = 1
const PTHREAD_EXPLICIT_SCHED = 2

const PTHREAD_CANCEL_ENABLE = &h01
const PTHREAD_CANCEL_DISABLE = &h00
const PTHREAD_CANCEL_DEFERRED = &h02
const PTHREAD_CANCEL_ASYNCHRONOUS = &h00
const PTHREAD_CANCELED = cptr(any ptr, 1)

const PTHREAD_SCOPE_SYSTEM = 1
const PTHREAD_SCOPE_PROCESS = 2
const PTHREAD_PROCESS_SHARED = 1
const PTHREAD_PROCESS_PRIVATE = 2

const PTHREAD_PRIO_NONE = 0
const PTHREAD_PRIO_INHERIT = 1
const PTHREAD_PRIO_PROTECT = 2

const PTHREAD_MUTEX_NORMAL = 0
const PTHREAD_MUTEX_ERRORCHECK = 1
const PTHREAD_MUTEX_RECURSIVE = 2
const PTHREAD_MUTEX_DEFAULT = PTHREAD_MUTEX_NORMAL
const PTHREAD_MUTEX_POLICY_FAIRSHARE_NP = 1
const PTHREAD_MUTEX_POLICY_FIRSTFIT_NP = 3

const PTHREAD_DESTRUCTOR_ITERATIONS = 4
const PTHREAD_KEYS_MAX = 512
#ifdef __FB_ARM__
	const PTHREAD_STACK_MIN = 16384
#else
	const PTHREAD_STACK_MIN = 8192
#endif

'' These signature values are part of Apple's public static initializer ABI.
const _PTHREAD_MUTEX_SIG_init = &h32AAABA7
const _PTHREAD_ERRORCHECK_MUTEX_SIG_init = &h32AAABA1
const _PTHREAD_RECURSIVE_MUTEX_SIG_init = &h32AAABA2
const _PTHREAD_COND_SIG_init = &h3CB0B1BB
const _PTHREAD_ONCE_SIG_init = &h30B1BCBA
const _PTHREAD_RWLOCK_SIG_init = &h2DA8B3B4

#define PTHREAD_MUTEX_INITIALIZER type<_opaque_pthread_mutex_t>(_PTHREAD_MUTEX_SIG_init, {0})
#define PTHREAD_ERRORCHECK_MUTEX_INITIALIZER type<_opaque_pthread_mutex_t>(_PTHREAD_ERRORCHECK_MUTEX_SIG_init, {0})
#define PTHREAD_RECURSIVE_MUTEX_INITIALIZER type<_opaque_pthread_mutex_t>(_PTHREAD_RECURSIVE_MUTEX_SIG_init, {0})
#define PTHREAD_COND_INITIALIZER type<_opaque_pthread_cond_t>(_PTHREAD_COND_SIG_init, {0})
#define PTHREAD_ONCE_INIT type<_opaque_pthread_once_t>(_PTHREAD_ONCE_SIG_init, {0})
#define PTHREAD_RWLOCK_INITIALIZER type<_opaque_pthread_rwlock_t>(_PTHREAD_RWLOCK_SIG_init, {0})

'' -------------------------------------------------------------------------
'' Thread lifecycle and attributes
'' -------------------------------------------------------------------------

extern "C"

declare function pthread_create _
	( _
		byval __newthread as pthread_t ptr, _
		byval __attr as const pthread_attr_t ptr, _
		byval __start_routine as function cdecl(byval as any ptr) as any ptr, _
		byval __arg as any ptr _
	) as long
declare sub pthread_exit(byval __retval as any ptr)
declare function pthread_join(byval __thread as pthread_t, byval __retval as any ptr ptr) as long
declare function pthread_detach(byval __thread as pthread_t) as long
declare function pthread_self() as pthread_t
declare function pthread_equal(byval __thread1 as pthread_t, byval __thread2 as pthread_t) as long

declare function pthread_attr_init(byval __attr as pthread_attr_t ptr) as long
declare function pthread_attr_destroy(byval __attr as pthread_attr_t ptr) as long
declare function pthread_attr_getdetachstate(byval __attr as const pthread_attr_t ptr, byval __state as long ptr) as long
declare function pthread_attr_setdetachstate(byval __attr as pthread_attr_t ptr, byval __state as long) as long
declare function pthread_attr_getguardsize(byval __attr as const pthread_attr_t ptr, byval __size as size_t ptr) as long
declare function pthread_attr_setguardsize(byval __attr as pthread_attr_t ptr, byval __size as size_t) as long
declare function pthread_attr_getinheritsched(byval __attr as const pthread_attr_t ptr, byval __inherit as long ptr) as long
declare function pthread_attr_setinheritsched(byval __attr as pthread_attr_t ptr, byval __inherit as long) as long
declare function pthread_attr_getschedparam(byval __attr as const pthread_attr_t ptr, byval __param as sched_param ptr) as long
declare function pthread_attr_setschedparam(byval __attr as pthread_attr_t ptr, byval __param as const sched_param ptr) as long
declare function pthread_attr_getschedpolicy(byval __attr as const pthread_attr_t ptr, byval __policy as long ptr) as long
declare function pthread_attr_setschedpolicy(byval __attr as pthread_attr_t ptr, byval __policy as long) as long
declare function pthread_attr_getscope(byval __attr as const pthread_attr_t ptr, byval __scope as long ptr) as long
declare function pthread_attr_setscope(byval __attr as pthread_attr_t ptr, byval __scope as long) as long
declare function pthread_attr_getstack(byval __attr as const pthread_attr_t ptr, byval __address as any ptr ptr, byval __size as size_t ptr) as long
declare function pthread_attr_setstack(byval __attr as pthread_attr_t ptr, byval __address as any ptr, byval __size as size_t) as long
declare function pthread_attr_getstackaddr(byval __attr as const pthread_attr_t ptr, byval __address as any ptr ptr) as long
declare function pthread_attr_setstackaddr(byval __attr as pthread_attr_t ptr, byval __address as any ptr) as long
declare function pthread_attr_getstacksize(byval __attr as const pthread_attr_t ptr, byval __size as size_t ptr) as long
declare function pthread_attr_setstacksize(byval __attr as pthread_attr_t ptr, byval __size as size_t) as long

'' -------------------------------------------------------------------------
'' Mutexes, condition variables, rwlocks, and once
'' -------------------------------------------------------------------------

declare function pthread_mutex_init(byval __mutex as pthread_mutex_t ptr, byval __attr as const pthread_mutexattr_t ptr) as long
declare function pthread_mutex_destroy(byval __mutex as pthread_mutex_t ptr) as long
declare function pthread_mutex_lock(byval __mutex as pthread_mutex_t ptr) as long
declare function pthread_mutex_trylock(byval __mutex as pthread_mutex_t ptr) as long
declare function pthread_mutex_unlock(byval __mutex as pthread_mutex_t ptr) as long
declare function pthread_mutex_getprioceiling(byval __mutex as const pthread_mutex_t ptr, byval __ceiling as long ptr) as long
declare function pthread_mutex_setprioceiling(byval __mutex as pthread_mutex_t ptr, byval __ceiling as long, byval __old_ceiling as long ptr) as long

declare function pthread_mutexattr_init(byval __attr as pthread_mutexattr_t ptr) as long
declare function pthread_mutexattr_destroy(byval __attr as pthread_mutexattr_t ptr) as long
declare function pthread_mutexattr_getprioceiling(byval __attr as const pthread_mutexattr_t ptr, byval __ceiling as long ptr) as long
declare function pthread_mutexattr_setprioceiling(byval __attr as pthread_mutexattr_t ptr, byval __ceiling as long) as long
declare function pthread_mutexattr_getprotocol(byval __attr as const pthread_mutexattr_t ptr, byval __protocol as long ptr) as long
declare function pthread_mutexattr_setprotocol(byval __attr as pthread_mutexattr_t ptr, byval __protocol as long) as long
declare function pthread_mutexattr_getpshared(byval __attr as const pthread_mutexattr_t ptr, byval __shared as long ptr) as long
declare function pthread_mutexattr_setpshared(byval __attr as pthread_mutexattr_t ptr, byval __shared as long) as long
declare function pthread_mutexattr_gettype(byval __attr as const pthread_mutexattr_t ptr, byval __kind as long ptr) as long
declare function pthread_mutexattr_settype(byval __attr as pthread_mutexattr_t ptr, byval __kind as long) as long
declare function pthread_mutexattr_getpolicy_np(byval __attr as const pthread_mutexattr_t ptr, byval __policy as long ptr) as long
declare function pthread_mutexattr_setpolicy_np(byval __attr as pthread_mutexattr_t ptr, byval __policy as long) as long

declare function pthread_cond_init(byval __cond as pthread_cond_t ptr, byval __attr as const pthread_condattr_t ptr) as long
declare function pthread_cond_destroy(byval __cond as pthread_cond_t ptr) as long
declare function pthread_cond_signal(byval __cond as pthread_cond_t ptr) as long
declare function pthread_cond_broadcast(byval __cond as pthread_cond_t ptr) as long
declare function pthread_cond_wait(byval __cond as pthread_cond_t ptr, byval __mutex as pthread_mutex_t ptr) as long
declare function pthread_cond_timedwait(byval __cond as pthread_cond_t ptr, byval __mutex as pthread_mutex_t ptr, byval __abstime as const timespec ptr) as long
declare function pthread_condattr_init(byval __attr as pthread_condattr_t ptr) as long
declare function pthread_condattr_destroy(byval __attr as pthread_condattr_t ptr) as long
declare function pthread_condattr_getpshared(byval __attr as const pthread_condattr_t ptr, byval __shared as long ptr) as long
declare function pthread_condattr_setpshared(byval __attr as pthread_condattr_t ptr, byval __shared as long) as long

declare function pthread_rwlock_init(byval __rwlock as pthread_rwlock_t ptr, byval __attr as const pthread_rwlockattr_t ptr) as long
declare function pthread_rwlock_destroy(byval __rwlock as pthread_rwlock_t ptr) as long
declare function pthread_rwlock_rdlock(byval __rwlock as pthread_rwlock_t ptr) as long
declare function pthread_rwlock_tryrdlock(byval __rwlock as pthread_rwlock_t ptr) as long
declare function pthread_rwlock_wrlock(byval __rwlock as pthread_rwlock_t ptr) as long
declare function pthread_rwlock_trywrlock(byval __rwlock as pthread_rwlock_t ptr) as long
declare function pthread_rwlock_unlock(byval __rwlock as pthread_rwlock_t ptr) as long
declare function pthread_rwlockattr_init(byval __attr as pthread_rwlockattr_t ptr) as long
declare function pthread_rwlockattr_destroy(byval __attr as pthread_rwlockattr_t ptr) as long
declare function pthread_rwlockattr_getpshared(byval __attr as const pthread_rwlockattr_t ptr, byval __shared as long ptr) as long
declare function pthread_rwlockattr_setpshared(byval __attr as pthread_rwlockattr_t ptr, byval __shared as long) as long

declare function pthread_once(byval __once_control as pthread_once_t ptr, byval __init_routine as sub cdecl()) as long

'' -------------------------------------------------------------------------
'' Cancellation, thread-local storage, and process hooks
'' -------------------------------------------------------------------------

declare function pthread_setcancelstate(byval __state as long, byval __old_state as long ptr) as long
declare function pthread_setcanceltype(byval __kind as long, byval __old_type as long ptr) as long
declare function pthread_cancel(byval __thread as pthread_t) as long
declare sub pthread_testcancel()

declare function pthread_key_create(byval __key as pthread_key_t ptr, byval __destructor as sub cdecl(byval as any ptr)) as long
declare function pthread_key_delete(byval __key as pthread_key_t) as long
declare function pthread_getspecific(byval __key as pthread_key_t) as any ptr
declare function pthread_setspecific(byval __key as pthread_key_t, byval __value as const any ptr) as long

declare function pthread_atfork(byval __prepare as sub cdecl(), byval __parent as sub cdecl(), byval __child as sub cdecl()) as long

'' -------------------------------------------------------------------------
'' Scheduling, names, and public Darwin extensions
'' -------------------------------------------------------------------------

declare function pthread_getconcurrency() as long
declare function pthread_setconcurrency(byval __level as long) as long
declare function pthread_getschedparam(byval __thread as pthread_t, byval __policy as long ptr, byval __param as sched_param ptr) as long
declare function pthread_setschedparam(byval __thread as pthread_t, byval __policy as long, byval __param as const sched_param ptr) as long

'' Unlike the Linux extension, Apple's setter always names the calling thread
'' and therefore accepts only the name.  The getter can inspect another thread.
declare function pthread_getname_np(byval __thread as pthread_t, byval __name as zstring ptr, byval __length as size_t) as long
declare function pthread_setname_np(byval __name as const zstring ptr) as long
declare function pthread_threadid_np(byval __thread as pthread_t, byval __thread_id as ulongint ptr) as long
declare function pthread_main_np() as long
declare function pthread_is_threaded_np() as long
declare function pthread_get_stacksize_np(byval __thread as pthread_t) as size_t
declare function pthread_get_stackaddr_np(byval __thread as pthread_t) as any ptr
declare sub pthread_yield_np()

declare function pthread_cond_signal_thread_np(byval __cond as pthread_cond_t ptr, byval __thread as pthread_t) as long
declare function pthread_cond_timedwait_relative_np(byval __cond as pthread_cond_t ptr, byval __mutex as pthread_mutex_t ptr, byval __relative_time as const timespec ptr) as long
declare function pthread_kill(byval __thread as pthread_t, byval __signal as long) as long
declare function pthread_sigmask(byval __operation as long, byval __new_set as const __sigset_t ptr, byval __old_set as __sigset_t ptr) as long
declare function pthread_cpu_number_np(byval __cpu_number as size_t ptr) as long

'' -------------------------------------------------------------------------
'' Quality of service
'' -------------------------------------------------------------------------

type qos_class_t as ulong
type pthread_override_t as any ptr

const QOS_CLASS_USER_INTERACTIVE = &h21
const QOS_CLASS_USER_INITIATED = &h19
const QOS_CLASS_DEFAULT = &h15
const QOS_CLASS_UTILITY = &h11
const QOS_CLASS_BACKGROUND = &h09
const QOS_CLASS_UNSPECIFIED = &h00
const QOS_MIN_RELATIVE_PRIORITY = -15

declare function qos_class_self() as qos_class_t
declare function qos_class_main() as qos_class_t
declare function pthread_attr_set_qos_class_np(byval __attr as pthread_attr_t ptr, byval __qos_class as qos_class_t, byval __relative_priority as long) as long
declare function pthread_attr_get_qos_class_np(byval __attr as pthread_attr_t ptr, byval __qos_class as qos_class_t ptr, byval __relative_priority as long ptr) as long
declare function pthread_set_qos_class_self_np(byval __qos_class as qos_class_t, byval __relative_priority as long) as long
declare function pthread_get_qos_class_np(byval __thread as pthread_t, byval __qos_class as qos_class_t ptr, byval __relative_priority as long ptr) as long
declare function pthread_override_qos_class_start_np(byval __thread as pthread_t, byval __qos_class as qos_class_t, byval __relative_priority as long) as pthread_override_t
declare function pthread_override_qos_class_end_np(byval __override as pthread_override_t) as long

end extern

'' -------------------------------------------------------------------------
'' Unsupported Linux interfaces
'' -------------------------------------------------------------------------

'' Darwin does not export the glibc try/timed join, CPU affinity, robust mutex,
'' mutex/rwlock timed wait, condition-clock attribute, spin lock, barrier, or
'' per-thread CPU clock interfaces.  They are deliberately not declared here;
'' a declaration would make invalid portable-looking code compile and then
'' either fail at link time or use an incompatible object layout.

'' Apple's pthread_cleanup_push() and pthread_cleanup_pop() are paired C macros
'' that open and close one lexical block while modifying private thread state.
'' FreeBASIC macros cannot expose that construct safely, so no misleading
'' function declarations are supplied.  Mach-thread conversion, suspended
'' creation, and JIT write-protection extensions are also left to dedicated
'' Mach or runtime bindings rather than making this common pthread header own
'' their additional types and process-wide policy.

#endif

'' end of crt/darwin/pthread.bi
