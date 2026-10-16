'' FreeBASIC classic AmigaOS shutdown qualification
'' ----------------------------------------------
'' File: thread-exit.bas
'' Purpose: Check that global destructors can stop and join their workers.
'' Responsibilities: Synchronize a stop request and verify joined worker exit.
'' This file intentionally does NOT contain: runtime shutdown implementation.

dim shared worker as any ptr, state_lock as any ptr
dim shared stop_requested as integer

private sub BackgroundWorker(byval unused as any ptr)
	do
		mutexlock(state_lock)
		dim stopping as integer = stop_requested
		mutexunlock(state_lock)
		if stopping then exit do
		sleep 1, 1
	loop
	print "NATIVE_THREAD_STOPPED"
end sub

private sub Cleanup() destructor
	if worker <> 0 then
		mutexlock(state_lock)
		stop_requested = -1
		mutexunlock(state_lock)
		threadwait(worker)
		worker = 0
		print "NATIVE_THREAD_DESTRUCTOR_OK"
	end if
	if state_lock <> 0 then mutexdestroy(state_lock)
	state_lock = 0
end sub

state_lock = mutexcreate()
if state_lock = 0 then end 1
worker = threadcreate(@BackgroundWorker)
if worker = 0 then end 2
print "NATIVE_THREAD_EXIT_READY"
end 0

'' end of thread-exit.bas
