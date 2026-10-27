'' TEST_MODE : COMPILE_ONLY_OK
'' Project: FreeBASIC compiler regression reduction
'' File: c-array-copy-failure-check.bas
'' Purpose: Observe an implicit dynamic-field copy after a failed resize.
'' Responsibilities: Check errors, empty destinations and repeated assignments.
'' This file intentionally does NOT replace allocation in the runtime archive.
#lang "fb"
#include once "fberror.bi"
declare function AllocationFailures cdecl alias "fb_CopyAllocationFailures" () as long
type sample
    leading as integer
    values(any) as integer
    trailing as integer
end type
dim as sample source_value, destination
source_value.leading = 17
source_value.trailing = 23
redim source_value.values(0 to 2)
for index as integer = 0 to 2
    source_value.values(index) = index + 101
next
destination.leading = -1
destination.trailing = -1
destination = source_value
if err <> cint(FB.FB_RTERROR_OUTOFMEM) then end 21
if AllocationFailures() <> 1 then end 22
if ubound(destination.values) <> -1 then end 23
if destination.leading <> 17 or destination.trailing <> 23 then end 24
for index as integer = 0 to 2
    if source_value.values(index) <> index + 101 then end 25
next

'' A failed copy must remain an empty source for the next assignment.
dim as sample repeated
repeated.leading = -1
repeated.trailing = -1
repeated = destination
if AllocationFailures() <> 1 then end 26
if err <> 0 then end 27
if ubound(repeated.values) <> -1 then end 28
if repeated.leading <> 17 or repeated.trailing <> 23 then end 29

print "ARRAY_COPY_FAILURE_PASS"
'' end of c-array-copy-failure-check.bas
