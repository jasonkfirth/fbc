/'
    Project: FreeBASIC C ERROR ownership qualification
    File: c-error-fixed-object-arrays.bas
    Purpose: Exercise generated managed-record lifetimes in fixed arrays.
    Responsibilities: Check interrupted initializers, early errors and local exits.
    This file intentionally does NOT exercise user destructors or dynamic object arrays.
'/
#lang "fb"

declare sub allocation_begin cdecl alias "allocation_begin"()
declare function allocation_end cdecl alias "allocation_end"() as long
declare sub finish_tls cdecl alias "finish_tls"()

dim shared as integer initializer_step, fail_step, handled_errors
dim shared as integer source_checks

private function field_text() as string
    initializer_step += 1
    if initializer_step = fail_step then error 24
    return "array field " & str(initializer_step) & string(48, "x")
end function

type ArrayChild
    text as string = field_text()
    words(0 to 1) as string
end type

type ArrayElement
    title as string = field_text()
    child as ArrayChild
    tail as string = field_text()
end type

private sub populate(byref item as ArrayElement, byval ordinal as integer)
    if (item.title <> "") = 0 then end 78
    if (item.child.text <> "") = 0 then end 78
    if (item.tail <> "") = 0 then end 78
    item.child.words(0) = "first " & str(ordinal)
    item.child.words(1) = "last " & str(ordinal)
    source_checks += 1
end sub

private sub worker(byval failure as integer)
    fail_step = failure
    initializer_step = 0
    if failure = 0 then error 24
    ' Nonzero lower bounds and two dimensions exercise the physical origin.
    dim items(-2 to 1, 3 to 4) as ArrayElement
    if (initializer_step = 24) = 0 then end 78
    for first as integer = -2 to 1
        for second as integer = 3 to 4
            populate(items(first, second), (first + 2) * 2 + second - 3)
        next
    next
    if failure = 25 then error 24
end sub

private function outer_owner(byval failure as integer) as integer
    on local error goto caught
    worker(failure)
    return 0
caught:
    if (err = 24) = 0 then end 78
    handled_errors += 1
    return err
end function

private function local_owner(byval failure as integer) as integer
    on local error goto caught
    fail_step = failure
    initializer_step = 0
    if failure = 0 then error 24
    scope
        dim items(-2 to 1, 3 to 4) as ArrayElement
        if (initializer_step = 24) = 0 then end 78
        for first as integer = -2 to 1
            for second as integer = 3 to 4
                populate(items(first, second), (first + 2) * 2 + second - 3)
            next
        next
        if failure = 25 then error 24
    end scope
    return 0
caught:
    if (err = 24) = 0 then end 78
    handled_errors += 1
    return err
end function

private sub repeated_scopes()
    fail_step = -1
    for repetition as integer = 1 to 3
        initializer_step = 0
        scope
            dim items(-2 to 1, 3 to 4) as ArrayElement
            if (initializer_step = 24) = 0 then end 78
            for first as integer = -2 to 1
                for second as integer = 3 to 4
                    populate(items(first, second), repetition)
                next
            next
            ' ERASE retires and reinitializes fixed generated records.
            erase items
            if (initializer_step = 48) = 0 then end 78
            if (items(-2, 3).child.words(0) = "") = 0 then end 78
        end scope
    next
end sub

worker(-1)
if outer_owner(1) <> 24 then end 79
if local_owner(1) <> 24 then end 80
repeated_scopes()
finish_tls()
handled_errors = 0
source_checks = 0
allocation_begin()
for repetition as integer = 1 to 4
    for failure as integer = 0 to 25
        if (outer_owner(failure) = 24) = 0 then end 78
        if (local_owner(failure) = 24) = 0 then end 78
    next
    if (outer_owner(-1) = 0) = 0 then end 78
    if (local_owner(-1) = 0) = 0 then end 78
    repeated_scopes()
next
if (handled_errors = 208) = 0 then end 78
if (source_checks = 224) = 0 then end 78
finish_tls()
if allocation_end() <> 0 then end 81
print "FIXED_OBJECT_ARRAYS_PASS"
end 0

' end of c-error-fixed-object-arrays.bas
