/'
    Project: FreeBASIC C continuation ownership qualification
    File: c-error-user-dtor-state.bas
    Purpose: Preserve user-destructor state independently of managed field presence.
    Responsibilities: Check skipped construction, interrupted bodies and normal exits.
    This file intentionally does NOT infer ownership of raw pointer fields.
'/
#lang "fb"

declare sub allocation_begin cdecl alias "allocation_begin"()
declare function allocation_end cdecl alias "allocation_end"() as long
declare sub finish_tls cdecl alias "finish_tls"()

dim shared failure_step as integer
dim shared destructor_failure as integer
dim shared destructor_calls as integer

type PlainState
    ready as integer
    declare constructor()
    declare destructor()
end type

constructor PlainState()
    if failure_step = 1 then error 24
    ready = -1
    '' A field value does not prove that the constructor has returned.
    if failure_step = 2 then error 24
end constructor

destructor PlainState()
    if ready <> -1 then end 92
    destructor_calls += 1
    if destructor_failure then
        destructor_failure = 0
        error 25
    end if
    err = 0
end destructor

private sub worker(byval normal_exit as integer)
    if failure_step = 0 and normal_exit = 0 then error 24
    for repetition as integer = 0 to 2
        scope
            dim state as PlainState
            if normal_exit = 0 then error 24
        end scope
    next
end sub

private function outside_owner() as integer
    on local error goto caught
    worker(0)
    return -1
caught:
    return err
end function

private function local_owner() as integer
    on local error goto caught
    if failure_step = 0 then error 24
    dim state as PlainState
    error 24
    return -1
caught:
    return err
end function

private function outside_dtor_owner() as integer
    on local error goto caught
    worker(-1)
    return -1
caught:
    return err
end function

private function local_dtor_owner() as integer
    on local error goto caught
    scope
        dim state as PlainState
    end scope
    return -1
caught:
    return err
end function

failure_step = -1
worker(-1)
finish_tls()
destructor_calls = 0
allocation_begin()
for iteration as integer = 0 to 7
    for failing as integer = 0 to 3
        failure_step = failing
        if outside_owner() <> 24 or err <> 24 then end 81
        if local_owner() <> 24 then end 82
        if failing = 3 then
            if err <> 0 then end 83
        else
            if err <> 24 then end 84
        end if
    next
    failure_step = -1
    worker(-1)
    destructor_failure = -1
    if outside_dtor_owner() <> 25 or err <> 25 then end 85
    destructor_failure = -1
    if local_dtor_owner() <> 25 or err <> 25 then end 86
next
finish_tls()
if destructor_calls <> 56 then end 87
if allocation_end() <> 0 then end 88
print "USER_DTOR_STATE_PASS"
end 0

' end of c-error-user-dtor-state.bas
