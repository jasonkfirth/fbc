/'
    Project: FreeBASIC C continuation ownership qualification
    File: c-error-user-ctor-handler.bas
    Purpose: Keep caller ownership live while a user constructor handles an error.
    Responsibilities: Check nested handlers, initialized fields and balanced retirement.
    This file intentionally does NOT define user destructors or dynamic object arrays.
'/
#lang "fb"

declare sub allocation_begin cdecl alias "allocation_begin"()
declare function allocation_end cdecl alias "allocation_end"() as long
declare sub finish_tls cdecl alias "finish_tls"()

dim shared constructor_errors as integer

type Resource
    first as string
    words(any) as string
    last as string
    declare constructor()
end type

constructor Resource()
    on local error goto caught
    first = "before handler"
    error 24
    exit constructor
caught:
    constructor_errors += 1
    redim words(0 to 1)
    words(1) = "after handler"
    last = "complete"
    exit constructor
end constructor

private sub check_value(byref state as Resource)
    if state.first <> "before handler" then end 81
    if state.words(1) <> "after handler" then end 82
    if state.last <> "complete" then end 83
end sub

private sub worker(byval fail_after as integer)
    dim state as Resource
    check_value(state)
    if fail_after then error 25
end sub

private function outside_owner() as integer
    on local error goto caught
    worker(-1)
    return -1
caught:
    return err
end function

private function local_owner() as integer
    on local error goto caught
    dim state as Resource
    check_value(state)
    error 25
    return -1
caught:
    return err
end function

worker(0)
finish_tls()
constructor_errors = 0
allocation_begin()
for iteration as integer = 0 to 15
    if outside_owner() <> 25 or err <> 25 then end 84
    if local_owner() <> 25 then end 85
    if err <> 0 then end 86
next
finish_tls()
if constructor_errors <> 32 then end 87
if allocation_end() <> 0 then end 88
print "USER_CTOR_HANDLER_PASS"
end 0

' end of c-error-user-ctor-handler.bas
