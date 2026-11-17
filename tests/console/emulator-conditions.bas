/'
    Project: FreeBASIC console qualification
    File: emulator-conditions.bas
    Purpose: Exercise condition signals and broadcasts with reusable workers.
    Responsibilities: Check protected predicates, waiter lifetime and shutdown.
    This file does not substitute polling for condition waits.
'/
#lang "fb"

const worker_count = 8
const round_count = 128
dim shared gate as any ptr
dim shared wake_workers as any ptr
dim shared wake_main as any ptr
dim shared ready_count as long
dim shared completed_count as long
dim shared generation as long

sub worker(byval unused as any ptr)
    for round_index as long = 1 to round_count
        mutexlock gate
        ready_count += 1
        condsignal wake_main
        while generation < round_index
            condwait wake_workers, gate
        wend
        completed_count += 1
        condsignal wake_main
        mutexunlock gate
    next
end sub

gate = mutexcreate()
wake_workers = condcreate()
wake_main = condcreate()
if gate = 0 or wake_workers = 0 or wake_main = 0 then end 71

dim handles(0 to worker_count - 1) as any ptr
for index as long = 0 to worker_count - 1
    handles(index) = threadcreate(@worker)
    if handles(index) = 0 then end 72
next

for round_index as long = 1 to round_count
    mutexlock gate
    while ready_count < round_index * worker_count
        condwait wake_main, gate
    wend
    generation = round_index
    condbroadcast wake_workers
    while completed_count < round_index * worker_count
        condwait wake_main, gate
    wend
    mutexunlock gate
next

for index as long = 0 to worker_count - 1
    threadwait handles(index)
next
if ready_count <> worker_count * round_count then end 73
if completed_count <> ready_count then end 74
conddestroy wake_main
conddestroy wake_workers
mutexdestroy gate
end 0

'' end of emulator-conditions.bas
