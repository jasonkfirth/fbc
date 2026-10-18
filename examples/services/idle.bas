'' FreeBASIC runtime examples: idle.bas
'' Purpose: Run a tiny finite polling loop with a monotonic deadline.
'' Responsibilities: Check statuses, bound waits and stop from the callback.
'' Network access and operating-system background scheduling are not included.
#lang "fb"
#include once "fbidle.bi"

function check_for_posts( byval user as any ptr ) as long
	dim checks as long ptr = cptr(long ptr, user)
	*checks += 1
	print "Post check "; *checks
	'' Replace this short operation with a nonblocking client-state check.
	'' Slow network work should run in a separately owned worker.
	return iif(*checks < 3, 1, 0)
end function

dim state as FB.IDLE_STATE
dim checks as long = 0
if FBC.idleInit(state, 250) <> FB.SERVICE_OK then end 1
while state.running
	if FBC.idleStep(state, @check_for_posts, @checks, 100) <> FB.SERVICE_OK then end 1
wend
print "Stopped after "; checks; " checks."
'' end of idle.bas
