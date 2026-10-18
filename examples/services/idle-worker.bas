'' FreeBASIC runtime examples: idle-worker.bas
'' Purpose: Keep a small polling worker alive beside a main application loop.
'' Responsibilities: Mutex-protected handoff, bounded shutdown and joining.
'' UI calls, notifications and network protocol details belong to the main app.
#lang "fb"
#include once "fbidle.bi"

type POLL_WORK
	mutex as any ptr
	checks as long
	stopping as long
	status as long
end type

function poll_once( byval user as any ptr ) as long
	dim work as POLL_WORK ptr = cptr(POLL_WORK ptr, user)
	mutexlock work->mutex
	work->checks += 1
	dim keep_running as long = iif(work->stopping = 0, 1, 0)
	mutexunlock work->mutex
	return keep_running
end function

sub poll_worker( byval user as any ptr )
	dim work as POLL_WORK ptr = cptr(POLL_WORK ptr, user)
	dim state as FB.IDLE_STATE
	dim status as long = FBC.idleInit(state, 100)
	while status = FB.SERVICE_OK andalso state.running
		mutexlock work->mutex
		dim stopping as long = work->stopping
		mutexunlock work->mutex
		if stopping then exit while
		status = FBC.idleStep(state, @poll_once, user, 25)
	wend
	mutexlock work->mutex
	work->status = status
	mutexunlock work->mutex
end sub

dim work as POLL_WORK
work.mutex = mutexcreate()
if work.mutex = 0 then end 1
dim worker as any ptr = threadcreate(@poll_worker, @work)
if worker = 0 then
	mutexdestroy work.mutex
	end 1
end if
'' Main-thread UI and notification calls can run here. The worker publishes
'' data under the mutex; it never calls a UI API through another thread's handle.
for tick as long = 1 to 100
	mutexlock work.mutex
	dim checks as long = work.checks
	mutexunlock work.mutex
	if checks >= 3 then exit for
	sleep 10, 1
next
mutexlock work.mutex
work.stopping = 1
mutexunlock work.mutex
threadwait worker
'' Joining ends all worker access before the mutex and stack data disappear.
mutexdestroy work.mutex
print "Worker checks "; work.checks; ", status "; work.status
if work.status <> FB.SERVICE_OK orelse work.checks < 3 then end 1
'' end of idle-worker.bas
