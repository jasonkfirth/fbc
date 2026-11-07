/'
    Project: FreeBASIC runtime regression tests
    File: runtime-array-queries.bas
    Purpose: Exercise query identity through ordinary array operations.
    Responsibilities: Check zero DATA, dimensions, preserved bytes and erase.
    This file intentionally does NOT construct descriptors or allocate large arrays.
'/
#lang "fb"

' A same-size resize normally retains storage. Bound the retry if it moves.
const RETRY_LIMIT as integer = 4
dim values() as ubyte
redim values(0 to 1)
values(0) = 17
values(1) = 25
dim lower as integer = cint(@values(0))
dim stable as integer = false
for attempt as integer = 1 to RETRY_LIMIT
    redim preserve values(lower to lower + 1)
    dim physical_base as integer = cint(@values(lower))
    if physical_base = lower then
        stable = true
        exit for
    end if
    lower = physical_base
next
if stable = false then
    erase values
    end 77
end if
if lbound(values) <> lower orelse ubound(values) <> lower + 1 then end 11
if lbound(values, 0) <> 1 orelse ubound(values, 0) <> 1 then end 12
if values(lower) <> 17 orelse values(lower + 1) <> 25 then end 13
dim minimum as integer = -cint(cuint(-1) shr 1) - 1
if lbound(values, minimum) <> 0 orelse ubound(values, minimum) <> -1 then end 14
erase values
if lbound(values) <> 0 orelse ubound(values) <> -1 then end 15
if ubound(values, 0) <> 0 then end 16
print "ARRAY_QUERY_BASIC=PASS"
end 0

' end of runtime-array-queries.bas
