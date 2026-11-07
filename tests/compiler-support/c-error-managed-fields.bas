/'
    Project: FreeBASIC managed ERROR ownership qualification
    File: c-error-managed-fields.bas
    Purpose: Retire generated managed aggregates after complete or interrupted initialization.
    Responsibilities: Exercise nested fixed fields, dynamic primitive fields and both owner storages.
    This file intentionally does NOT construct dynamic object arrays or user-defined special members.
'/
#lang "fb"

declare sub allocation_begin cdecl alias "allocation_begin"()
declare function allocation_end cdecl alias "allocation_end"() as long
declare sub finish_tls cdecl alias "finish_tls"()

dim shared failure_step as integer
dim shared initialized_fields as integer

private function initialize_text() as string
    initialized_fields += 1
    if initialized_fields = failure_step then error 24
    return "managed field " & str(initialized_fields)
end function

type InnerState
    text as string = initialize_text()
    fixed_words(0 to 1) as string
#ifdef TEST_BOUNDED_FIELDS
    redim numbers(-2 to 2) as integer
    redim words(2 to 4) as string
#else
    numbers(any) as integer
    words(any) as string
#endif
end type

type ManagedState
    first as string = initialize_text()
    child as InnerState
    children(0 to 1) as InnerState
    last as string = initialize_text()
end type

private sub populate_inner(byref state as InnerState)
    redim state.numbers(-2 to 2)
    redim state.words(2 to 4)
    state.numbers(0) = 41
    state.words(3) = "dynamic field"
    state.fixed_words(1) = "fixed field"
end sub

private sub populate(byref state as ManagedState)
    populate_inner(state.child)
    populate_inner(state.children(0))
    state.children(1).fixed_words(0) = "nested fixed field"
end sub

private sub worker(byval normal_exit as integer)
    if failure_step = 0 and normal_exit = 0 then error 24
    dim state as ManagedState
    populate(state)
    if normal_exit = 0 then error 24
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
    dim state as ManagedState
    populate(state)
    error 24
    return -1
caught:
    return err
end function

' Complete construction warms the temporary-string descriptor pool before
' allocation accounting starts. Later partial paths need no larger pool.
failure_step = -1
initialized_fields = 0
worker(-1)
finish_tls()
allocation_begin()
for iteration as integer = 0 to 7
    for failing as integer = 0 to 6
        failure_step = failing
        initialized_fields = 0
        if outside_owner() <> 24 then end 81
        if err <> 24 then end 82
        initialized_fields = 0
        if local_owner() <> 24 then end 83
        ' Returning normally runs the owner's array field destruction.
        ' Its successful ERASE retains the predecessor's ordinary ERR=0.
        if err <> 0 then end 84
    next
    failure_step = -1
    initialized_fields = 0
    worker(-1)
next
finish_tls()
if allocation_end() <> 0 then end 85
print "MANAGED_FIELDS_OWNERSHIP_PASS"
end 0

' end of c-error-managed-fields.bas
