/'
    Project: FreeBASIC C continuation ownership qualification
    File: c-error-user-constructors.bas
    Purpose: Retire zero-safe fields when a user default constructor stops early.
    Responsibilities: Exercise nested construction, scope reentry and both owner storages.
    This file intentionally does NOT define user destructors or dynamic object arrays.
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
    return "user field " & str(initialized_fields)
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
    declare constructor()
end type

constructor ManagedState()
    first = initialize_text()
    last = initialize_text()
end constructor

private sub worker(byval normal_exit as integer)
    if failure_step = 0 and normal_exit = 0 then error 24
    for repetition as integer = 0 to 2
        scope
            dim state as ManagedState
            if state.child.numbers(0) <> 41 then end 86
            if state.children(1).words(3) <> "dynamic field" then end 87
            if len(state.child.fixed_words(1)) = 0 then end 88
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

'' Warm every ordinary allocation path before measuring the error cases.
failure_step = -1
initialized_fields = 0
worker(-1)
finish_tls()
allocation_begin()
for iteration as integer = 0 to 7
    for failing as integer = 0 to 9
        failure_step = failing
        initialized_fields = 0
        if outside_owner() <> 24 then end 81
        if err <> 24 then end 82
        initialized_fields = 0
        if local_owner() <> 24 then end 83
        '' Successful ordinary array-field retirement retains ERR=0.
        if err <> 0 then end 84
    next
    failure_step = -1
    initialized_fields = 0
    worker(-1)
next
finish_tls()
if allocation_end() <> 0 then end 85
print "USER_CONSTRUCTOR_FIELDS_PASS"
end 0

' end of c-error-user-constructors.bas
