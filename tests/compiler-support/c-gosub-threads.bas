/'
    Project: FreeBASIC local C GOSUB qualification
    File: c-gosub-threads.bas
    Purpose: Check independent GOSUB contexts and ERR values in four workers.
    Responsibilities: Join all workers and verify their private managed values.
    This file intentionally does NOT share a GOSUB context between threads.
'/
#lang "fblite"
option gosub

type WorkerState
    identity as integer
    result_value as integer
end type

' The main thread holds this gate until every worker has been created.
' Each worker owns one result record; ThreadWait precedes every result read.
dim shared start_gate as any ptr

private sub worker(byval opaque as any ptr)
    dim state as WorkerState ptr = opaque
    dim level as integer
    dim total as integer
    dim text_value as string
    MutexLock(start_gate)
    MutexUnlock(start_gate)
    for iteration as integer = 1 to 64
        level = 0
        total = 0
        text_value = "worker"
        err = 30 + state->identity
        gosub descend
        if level <> 0 or total <> 561 then exit sub
        if text_value <> "worker-done" then exit sub
        if err <> 30 + state->identity then exit sub
        state->result_value += total
    next
    exit sub
descend:
    level += 1
    if level < 33 then gosub descend
    total += level
    if level = 1 then text_value += "-done"
    level -= 1
    return
end sub

dim states(0 to 3) as WorkerState
dim handles(0 to 3) as any ptr
start_gate = MutexCreate()
if start_gate = 0 then end 71
MutexLock(start_gate)
dim created as integer
dim index as integer
for index = 0 to 3
    states(index).identity = index
    handles(index) = ThreadCreate(@worker, @states(index))
    if handles(index) = 0 then exit for
    created += 1
next
MutexUnlock(start_gate)
for index = 0 to created - 1
    ThreadWait(handles(index))
next
MutexDestroy(start_gate)
if created <> 4 then end 72
for index = 0 to 3
    if states(index).result_value <> 35904 then end 73
next
print "GOSUB_THREADS_PASS"
end 0

' end of c-gosub-threads.bas
