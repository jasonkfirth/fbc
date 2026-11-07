/'
    Project: FreeBASIC C continuation ownership qualification
    File: c-error-managed-destructors.bas
    Purpose: Retire managed fields without repeating an unfinished user destructor.
    Responsibilities: Check partial construction, ordinary destruction and owner exits.
    This file intentionally does NOT use nested user destructors or object arrays.
'/
#lang "fb"

declare sub allocation_begin cdecl alias "allocation_begin"()
declare function allocation_end cdecl alias "allocation_end"() as long
declare sub finish_tls cdecl alias "finish_tls"()

dim shared failure_step as integer
dim shared initialized_fields as integer
dim shared destructor_calls as integer
dim shared destructor_failure as integer

private function initialize_text() as string
    initialized_fields += 1
    if initialized_fields = failure_step then error 24
    return "managed field " & str(initialized_fields)
end function

type InnerState
    text as string
    fixed_words(0 to 1) as string
    numbers(any) as integer
    words(any) as string
    declare constructor()
end type

constructor InnerState()
    text = initialize_text()
    redim numbers(-2 to 2)
    redim words(2 to 4)
    numbers(0) = 41
    words(3) = "dynamic field"
    fixed_words(1) = initialize_text()
end constructor

type ManagedState
    first as string
    child as InnerState
    children(0 to 1) as InnerState
    last as string
    ready as integer
    declare constructor()
    declare destructor()
end type

constructor ManagedState()
    first = initialize_text()
    last = initialize_text()
    '' The user body must never see an unfinished instance during cleanup.
    ready = 12345
end constructor

destructor ManagedState()
    if ready <> 12345 then end 91
    destructor_calls += 1
    first = ""
    erase child.numbers
    if destructor_failure then
        destructor_failure = 0
        error 25
    end if
end destructor

private sub worker(byval normal_exit as integer)
    if failure_step = 0 and normal_exit = 0 then error 24
    for repetition as integer = 0 to 2
        scope
            dim state as ManagedState
            if state.child.numbers(0) <> 41 then end 92
            if state.children(1).words(3) <> "dynamic field" then end 93
            if len(state.child.fixed_words(1)) = 0 then end 94
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
    dim state as ManagedState
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
        dim state as ManagedState
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
    for failing as integer = 0 to 9
        failure_step = failing
        initialized_fields = 0
        if outside_owner() <> 24 or err <> 24 then end 81
        initialized_fields = 0
        if local_owner() <> 24 then end 82
        '' Before DIM there is no object to destroy or successful ERASE to
        '' replace ERR. Otherwise ordinary field retirement leaves ERR=0.
        if failing = 0 then
            if err <> 24 then end 83
        else
            if err <> 0 then end 84
        end if
    next
    failure_step = -1
    initialized_fields = 0
    worker(-1)
    destructor_failure = -1
    if outside_dtor_owner() <> 25 or err <> 25 then end 85
    destructor_failure = -1
    if local_dtor_owner() <> 25 then end 86
    if err <> 0 then end 87
next
finish_tls()
if destructor_calls <> 56 then end 88
if allocation_end() <> 0 then end 89
print "MANAGED_DESTRUCTORS_PASS"
end 0

' end of c-error-managed-destructors.bas
