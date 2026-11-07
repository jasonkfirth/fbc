/'
    Project: FreeBASIC C continuation ownership qualification
    File: c-error-owner-final-dtor.bas
    Purpose: Keep the owner registered until its pending object retirement finishes.
    Responsibilities: Check nested handler selection, destructor reentry and heap balance.
    This file intentionally does NOT recover a second error during abandoned-frame cleanup.
'/
#lang "fb"

declare sub allocation_begin cdecl alias "allocation_begin"()
declare function allocation_end cdecl alias "allocation_end"() as long
declare sub finish_tls cdecl alias "finish_tls"()

dim shared fail_in_destructor as integer
dim shared destructor_calls as integer

type Resource
    text as string
    words(any) as string
    ready as integer
    declare constructor()
    declare destructor()
end type

constructor Resource()
    text = "pending owner storage"
    redim words(0 to 1)
    words(1) = "second owning field"
    ready = -1
end constructor

destructor Resource()
    if ready <> -1 then end 91
    destructor_calls += 1
    erase words
    if fail_in_destructor then
        fail_in_destructor = 0
        error 25
    end if
end destructor

private function inner_owner() as integer
    on local error goto caught
    scope
        dim state as Resource
        error 24
    end scope
    return -1
caught:
    return err
end function

private function outer_owner() as integer
    on local error goto caught
    return inner_owner()
caught:
    return err
end function

outer_owner()
finish_tls()
destructor_calls = 0
allocation_begin()
for iteration as integer = 0 to 15
    if outer_owner() <> 24 or err <> 0 then end 81
    fail_in_destructor = -1
    if outer_owner() <> 25 or err <> 25 then end 82
next
finish_tls()
if destructor_calls <> 32 then end 83
if allocation_end() <> 0 then end 84
print "OWNER_FINAL_DESTRUCTOR_PASS"
end 0

' end of c-error-owner-final-dtor.bas
