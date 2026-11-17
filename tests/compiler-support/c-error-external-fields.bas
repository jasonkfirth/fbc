/'
    Project: FreeBASIC C-backend regression tests
    File: c-error-external-fields.bas
    Purpose: Retain generated child destructors used only by ERROR cleanup.
    Responsibilities: Check local and outer owners with separate member bodies.
    The provider owns user members, while ordinary compiler cleanup owns fields.
'/
#lang "fb"
#include once "c-error-external-fields.bi"
using CleanupFields

#ifdef AUDIT_ALLOCATIONS
declare sub allocation_begin cdecl alias "allocation_begin"()
declare function allocation_end cdecl alias "allocation_end"() as long
declare sub finish_tls cdecl alias "finish_tls"()
#endif

private sub worker()
    dim value as ParentState
    if value.child.text <> "scalar child" then end 91
    if value.children(1).words(0) <> "last child" then end 92
end sub

private function outside_owner() as long
    on local error goto caught
    worker()
    return 0
caught:
    return err
end function

private function local_owner() as long
    on local error goto caught
    scope
        dim value as ParentState
        if value.child.words(1) <> "scalar array field" then end 93
    end scope
    return 0
caught:
    return err
end function

'' Warm ordinary runtime and handler storage before observing allocation balance.
if outside_owner() <> 0 or local_owner() <> 0 then end 94
#ifdef AUDIT_ALLOCATIONS
finish_tls()
allocation_begin()
#endif
constructor_calls = 0
destructor_calls = 0
for repetition as long = 0 to 7
    for selected as long = 0 to 2
        dim expected as long = 0
        if selected <> 0 then expected = 23 + selected
        failure_kind = selected
        if outside_owner() <> expected then end 95
        failure_kind = selected
        if local_owner() <> expected then end 96
    next
next
if constructor_calls <> 48 or destructor_calls <> 32 then end 97
#ifdef AUDIT_ALLOCATIONS
finish_tls()
if allocation_end() <> 0 then end 98
#endif
print "ERROR_EXTERNAL_FIELDS_PASS"

' end of c-error-external-fields.bas
