/'
    Project: FreeBASIC C continuation ownership qualification
    File: c-error-copy-owners.bas
    Purpose: Retire scalar copies without losing their source or repeating user destruction.
    Responsibilities: Check generated copy construction, interrupted LET and owner exits.
    This file intentionally does NOT admit user copy constructors or object arrays.
'/
#lang "fb"

declare sub allocation_begin cdecl alias "allocation_begin"()
declare function allocation_end cdecl alias "allocation_end"() as long
declare sub finish_tls cdecl alias "finish_tls"()

dim shared failure_step as integer
dim shared copied_fields as integer
dim shared destructor_calls as integer
dim shared destructor_failure as integer

private sub copy_checkpoint()
    copied_fields += 1
    if copied_fields = failure_step then error 24
end sub

type ChildState
    text as string
    words(0 to 1) as string
    number as integer
end type

type CopyState
    title as string
    child as ChildState
    children(0 to 1) as ChildState
    tail as string
    ready as integer
    declare operator let(byref rhs as const CopyState)
    declare destructor()
end type

operator CopyState.let(byref rhs as const CopyState)
    if @this = @rhs then exit operator
    title = rhs.title
    copy_checkpoint()
    child = rhs.child
    copy_checkpoint()
    children(0) = rhs.children(0)
    copy_checkpoint()
    children(1) = rhs.children(1)
    copy_checkpoint()
    tail = rhs.tail
    copy_checkpoint()
    ready = rhs.ready
    '' Even after this final assignment, a raised error leaves construction
    '' incomplete. The user destructor must not infer completion from ready.
    copy_checkpoint()
end operator

destructor CopyState()
    if ready <> 12345 then end 91
    destructor_calls += 1
    erase child.words
    if destructor_failure then
        destructor_failure = 0
        error 25
    end if
end destructor

type SeededState
    text as string
    number as integer
    declare constructor(byval seed as integer)
end type

constructor SeededState(byval seed as integer)
    text = "source without a default constructor"
    number = seed
end constructor

dim shared source_state as CopyState

private sub check_source()
    if source_state.title <> "source title" then end 92
    if source_state.child.text <> "source child" then end 93
    if source_state.child.words(1) <> "source fixed word" then end 94
    if source_state.children(0).text <> "first child" then end 95
    if source_state.children(1).words(1) <> "last fixed word" then end 96
    if source_state.tail <> "source tail" or source_state.ready <> 12345 then end 97
end sub

private sub copy_worker(byval normal_exit as integer)
    if failure_step = 0 and normal_exit = 0 then error 24
    for repetition as integer = 0 to 2
        scope
            dim copied as CopyState = source_state
            if copied.child.number <> 41 then end 71
            if copied.children(1).words(1) <> "last fixed word" then end 72
            if normal_exit = 0 then error 24
        end scope
    next
end sub

private function outside_owner() as integer
    on local error goto caught
    copy_worker(0)
    return -1
caught:
    return err
end function

private function local_owner() as integer
    on local error goto caught
    if failure_step = 0 then error 24
    dim copied as CopyState = source_state
    error 24
    return -1
caught:
    return err
end function

private function outside_dtor_owner() as integer
    on local error goto caught
    copy_worker(-1)
    return -1
caught:
    return err
end function

private function local_dtor_owner() as integer
    on local error goto caught
    scope
        dim copied as CopyState = source_state
    end scope
    return -1
caught:
    return err
end function

private function seeded_owner(byref original as const SeededState) as integer
    on local error goto caught
    dim copied as SeededState = original
    if copied.number <> 91 then end 73
    error 24
    return -1
caught:
    return err
end function

source_state.title = "source title"
source_state.child.text = "source child"
source_state.child.words(1) = "source fixed word"
source_state.child.number = 41
source_state.children(0).text = "first child"
source_state.children(1).words(1) = "last fixed word"
source_state.tail = "source tail"
source_state.ready = 12345
dim seeded_source as SeededState = SeededState(91)

failure_step = -1
copy_worker(-1)
failure_step = 1
copied_fields = 0
outside_owner()
finish_tls()
destructor_calls = 0
allocation_begin()
for iteration as integer = 0 to 7
    for step_number as integer = 0 to 7
        failure_step = step_number
        copied_fields = 0
        if outside_owner() <> 24 then end 81
        check_source()
        copied_fields = 0
        if local_owner() <> 24 then end 82
        check_source()
    next
    failure_step = -1
    copied_fields = 0
    copy_worker(-1)
    destructor_failure = -1
    if outside_dtor_owner() <> 25 then end 83
    check_source()
    destructor_failure = -1
    if local_dtor_owner() <> 25 then end 84
    check_source()
    if seeded_owner(seeded_source) <> 24 then end 85
    if seeded_source.number <> 91 or seeded_source.text <> "source without a default constructor" then end 86
next
finish_tls()
if destructor_calls <> 56 then end 87
if allocation_end() <> 0 then end 88
print "SCALAR_COPY_OWNERS_PASS"
end 0

' end of c-error-copy-owners.bas
