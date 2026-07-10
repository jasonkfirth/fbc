''
'' FreeBASIC macOS pthread smoke test
'' ----------------------------------
''
'' File: pthread-darwin-smoke.bas
''
'' Purpose:
''
''     Verify the Darwin pthread ABI and the common interfaces in libSystem.
''
'' Responsibilities:
''
''     - assert SDK-derived opaque object sizes, offsets, and constants
''     - exercise thread creation, joining, attributes, names, and scheduling
''     - exercise static mutex, condition, rwlock, once, and TLS objects
''
'' This file intentionally does NOT contain:
''
''     - timing-dependent sleeps or polling
''     - Linux-only pthread extensions
''     - cancellation tests that can leave cleanup state behind
''

#include once "crt/pthread.bi"

#ifndef __FB_DARWIN__
	#error "pthread-darwin-smoke.bas requires the Darwin target"
#endif

#ifndef __FB_64BIT__
	#error "pthread-darwin-smoke.bas requires a 64-bit Darwin target"
#endif

const SMOKE_OK = 0
const SMOKE_ABI_FAILED = 1
const SMOKE_ATTRIBUTE_FAILED = 2
const SMOKE_THREAD_FAILED = 3
const SMOKE_SYNCHRONIZATION_FAILED = 4

type PthreadAlignmentProbe
	prefix as ubyte
	value as pthread_mutex_t
end type

#assert sizeof(pthread_t) = 8
#assert sizeof(pthread_key_t) = 8
#assert sizeof(_opaque_pthread_t) = 8192
#assert offsetof(_opaque_pthread_t, __cleanup_stack) = 8
#assert offsetof(_opaque_pthread_t, __opaque(0)) = 16

#assert sizeof(pthread_attr_t) = 64
#assert sizeof(pthread_mutex_t) = 64
#assert sizeof(pthread_mutexattr_t) = 16
#assert sizeof(pthread_cond_t) = 48
#assert sizeof(pthread_condattr_t) = 16
#assert sizeof(pthread_once_t) = 16
#assert sizeof(pthread_rwlock_t) = 200
#assert sizeof(pthread_rwlockattr_t) = 24
#assert offsetof(pthread_mutex_t, __opaque(0)) = 8
#assert offsetof(PthreadAlignmentProbe, value) = 8

#assert PTHREAD_CREATE_JOINABLE = 1
#assert PTHREAD_CREATE_DETACHED = 2
#assert PTHREAD_PROCESS_SHARED = 1
#assert PTHREAD_PROCESS_PRIVATE = 2
#assert PTHREAD_CANCELED = cptr(any ptr, 1)
#ifdef __FB_ARM__
	#assert PTHREAD_STACK_MIN = 16384
#else
	#assert PTHREAD_STACK_MIN = 8192
#endif
#assert QOS_CLASS_USER_INTERACTIVE = &h21
#assert QOS_CLASS_BACKGROUND = &h09
#assert QOS_MIN_RELATIVE_PRIORITY = -15

dim shared thread_mutex as pthread_mutex_t = PTHREAD_MUTEX_INITIALIZER
dim shared thread_condition as pthread_cond_t = PTHREAD_COND_INITIALIZER
dim shared thread_once as pthread_once_t = PTHREAD_ONCE_INIT
dim shared thread_rwlock as pthread_rwlock_t = PTHREAD_RWLOCK_INITIALIZER
dim shared recursive_mutex as pthread_mutex_t = PTHREAD_RECURSIVE_MUTEX_INITIALIZER

dim shared thread_key as pthread_key_t
dim shared worker_ready as long
dim shared worker_value as long
dim shared worker_failures as long
dim shared once_invocations as long
dim shared tls_destructor_invocations as long

sub InitializeOnce cdecl()
	once_invocations += 1
end sub

sub DestroyThreadValue cdecl(byval value as any ptr)
	if( value <> 0 ) then
		tls_destructor_invocations += 1
	end if
end sub

function RunWorker cdecl(byval argument as any ptr) as any ptr
	dim as long local_failures
	dim as ulongint thread_id
	dim as qos_class_t qos_class
	dim as long relative_priority
	dim as zstring * 64 thread_name

	if( pthread_once(@thread_once, @InitializeOnce) <> 0 ) then
		local_failures += 1
	end if
	if( pthread_once(@thread_once, @InitializeOnce) <> 0 ) then
		local_failures += 1
	end if

	if( pthread_setspecific(thread_key, argument) <> 0 ) then
		local_failures += 1
	end if
	if( pthread_getspecific(thread_key) <> argument ) then
		local_failures += 1
	end if

	if( pthread_setname_np("fbc-pthread-smoke") <> 0 ) then
		local_failures += 1
	end if
	if( pthread_getname_np(pthread_self(), @thread_name, sizeof(thread_name)) <> 0 ) then
		local_failures += 1
	else
		if( thread_name <> "fbc-pthread-smoke" ) then
			local_failures += 1
		end if
	end if

	if( pthread_threadid_np(pthread_self(), @thread_id) <> 0 orelse thread_id = 0 ) then
		local_failures += 1
	end if
	if( pthread_get_qos_class_np(pthread_self(), @qos_class, @relative_priority) <> 0 ) then
		local_failures += 1
	end if

	if( pthread_mutex_lock(@thread_mutex) <> 0 ) then
		return 0
	end if

	worker_value = *cptr(long ptr, argument)
	worker_failures = local_failures
	worker_ready = TRUE

	if( pthread_cond_signal(@thread_condition) <> 0 ) then
		worker_failures += 1
	end if
	if( pthread_mutex_unlock(@thread_mutex) <> 0 ) then
		return 0
	end if

	return argument
end function

'' The signatures below prove that the aggregate macros did not silently turn
'' into zero-filled objects, which Darwin interprets differently from a static
'' pthread initializer.
if( thread_mutex.__sig <> _PTHREAD_MUTEX_SIG_init or _
    thread_condition.__sig <> _PTHREAD_COND_SIG_init or _
    thread_once.__sig <> _PTHREAD_ONCE_SIG_init or _
    thread_rwlock.__sig <> _PTHREAD_RWLOCK_SIG_init or _
    recursive_mutex.__sig <> _PTHREAD_RECURSIVE_MUTEX_SIG_init ) then
	end SMOKE_ABI_FAILED
end if

dim thread_attributes as pthread_attr_t
dim stack_size as size_t
dim detach_state as long
dim scope_kind as long
dim scheduling_policy as long
dim scheduling_parameters as sched_param
dim attribute_qos_class as qos_class_t
dim attribute_relative_priority as long

if( pthread_attr_init(@thread_attributes) <> 0 ) then
	end SMOKE_ATTRIBUTE_FAILED
end if
if( pthread_attr_getstacksize(@thread_attributes, @stack_size) <> 0 or _
    stack_size < PTHREAD_STACK_MIN ) then
	pthread_attr_destroy(@thread_attributes)
	end SMOKE_ATTRIBUTE_FAILED
end if
if( pthread_attr_getdetachstate(@thread_attributes, @detach_state) <> 0 or _
    detach_state <> PTHREAD_CREATE_JOINABLE ) then
	pthread_attr_destroy(@thread_attributes)
	end SMOKE_ATTRIBUTE_FAILED
end if
if( pthread_attr_getscope(@thread_attributes, @scope_kind) <> 0 or _
    scope_kind <> PTHREAD_SCOPE_SYSTEM ) then
	pthread_attr_destroy(@thread_attributes)
	end SMOKE_ATTRIBUTE_FAILED
end if
if( pthread_attr_getschedpolicy(@thread_attributes, @scheduling_policy) <> 0 or _
    pthread_attr_getschedparam(@thread_attributes, @scheduling_parameters) <> 0 ) then
	pthread_attr_destroy(@thread_attributes)
	end SMOKE_ATTRIBUTE_FAILED
end if
if( pthread_attr_get_qos_class_np( _
		@thread_attributes, _
		@attribute_qos_class, _
		@attribute_relative_priority _
	) <> 0 ) then
	pthread_attr_destroy(@thread_attributes)
	end SMOKE_ATTRIBUTE_FAILED
end if

if( pthread_key_create(@thread_key, @DestroyThreadValue) <> 0 ) then
	pthread_attr_destroy(@thread_attributes)
	end SMOKE_THREAD_FAILED
end if

dim worker_thread as pthread_t
dim returned_value as any ptr
dim expected_value as long = &h12345678

if( pthread_mutex_lock(@thread_mutex) <> 0 ) then
	pthread_key_delete(thread_key)
	pthread_attr_destroy(@thread_attributes)
	end SMOKE_SYNCHRONIZATION_FAILED
end if

if( pthread_create( _
		@worker_thread, _
		@thread_attributes, _
		@RunWorker, _
		@expected_value _
	) <> 0 ) then
	pthread_mutex_unlock(@thread_mutex)
	pthread_key_delete(thread_key)
	pthread_attr_destroy(@thread_attributes)
	end SMOKE_THREAD_FAILED
end if

if( pthread_attr_destroy(@thread_attributes) <> 0 ) then
	pthread_mutex_unlock(@thread_mutex)
	pthread_join(worker_thread, 0)
	pthread_key_delete(thread_key)
	end SMOKE_ATTRIBUTE_FAILED
end if

while( worker_ready = FALSE )
	if( pthread_cond_wait(@thread_condition, @thread_mutex) <> 0 ) then
		pthread_mutex_unlock(@thread_mutex)
		pthread_join(worker_thread, 0)
		pthread_key_delete(thread_key)
		end SMOKE_SYNCHRONIZATION_FAILED
	end if
wend

if( pthread_mutex_unlock(@thread_mutex) <> 0 ) then
	pthread_join(worker_thread, 0)
	pthread_key_delete(thread_key)
	end SMOKE_SYNCHRONIZATION_FAILED
end if

if( pthread_join(worker_thread, @returned_value) <> 0 ) then
	pthread_key_delete(thread_key)
	end SMOKE_THREAD_FAILED
end if

if( returned_value <> @expected_value or worker_value <> expected_value or _
    worker_failures <> 0 or tls_destructor_invocations <> 1 ) then
	pthread_key_delete(thread_key)
	end SMOKE_THREAD_FAILED
end if

if( pthread_once(@thread_once, @InitializeOnce) <> 0 or once_invocations <> 1 ) then
	pthread_key_delete(thread_key)
	end SMOKE_SYNCHRONIZATION_FAILED
end if
if( pthread_key_delete(thread_key) <> 0 ) then
	end SMOKE_THREAD_FAILED
end if

if( pthread_rwlock_rdlock(@thread_rwlock) <> 0 or _
    pthread_rwlock_unlock(@thread_rwlock) <> 0 or _
    pthread_rwlock_wrlock(@thread_rwlock) <> 0 or _
    pthread_rwlock_unlock(@thread_rwlock) <> 0 ) then
	end SMOKE_SYNCHRONIZATION_FAILED
end if

if( pthread_mutex_lock(@recursive_mutex) <> 0 or _
    pthread_mutex_lock(@recursive_mutex) <> 0 or _
    pthread_mutex_unlock(@recursive_mutex) <> 0 or _
    pthread_mutex_unlock(@recursive_mutex) <> 0 ) then
	end SMOKE_SYNCHRONIZATION_FAILED
end if

if( pthread_rwlock_destroy(@thread_rwlock) <> 0 or _
    pthread_cond_destroy(@thread_condition) <> 0 or _
    pthread_mutex_destroy(@recursive_mutex) <> 0 or _
    pthread_mutex_destroy(@thread_mutex) <> 0 ) then
	end SMOKE_SYNCHRONIZATION_FAILED
end if

end SMOKE_OK

'' end of pthread-darwin-smoke.bas
