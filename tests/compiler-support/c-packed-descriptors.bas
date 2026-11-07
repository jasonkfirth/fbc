/'
    Project: FreeBASIC C backend regression tests
    File: c-packed-descriptors.bas
    Purpose: Preserve packed managed record layout across C and runtime calls.
    Responsibilities: Check field positions, string values and dynamic array lifetime.
    This file intentionally does NOT use ERROR handlers or partial construction.
'/
#lang "fb"

type InnerState field = 1
    tag as ubyte
    text as string
    words(0 to 1) as string
    values(any) as integer
end type

type OuterState field = 1
    prefix as ubyte
    child as InnerState
    suffix as ubyte
end type

private sub exercise()
    dim state as OuterState

    '' The leading byte occupies one naturally padded pointer word, followed
    '' by three STRING descriptors and a one-dimensional array descriptor.
    if sizeof(InnerState) <> 19 * sizeof(any ptr) then end 81
    if sizeof(OuterState) <> sizeof(InnerState) + 2 then end 82
    if cuint(@state.child) - cuint(@state) <> 1 then end 83

    state.prefix = 17
    state.suffix = 23
    state.child.text = "packed child"
    state.child.words(1) = "fixed field"
    redim state.child.values(0 to 1)
    state.child.values(1) = 41
    if state.child.text <> "packed child" then end 84
    if state.child.words(1) <> "fixed field" then end 85
    if state.child.values(1) <> 41 then end 86
    if state.prefix <> 17 or state.suffix <> 23 then end 87
end sub

exercise()
#ifdef TEST_ALIGNMENT
    '' Query after the ordinary local destructor has released every field.
    declare function descriptor_alignment_errors cdecl alias "descriptor_alignment_errors"() as ulong
    if descriptor_alignment_errors() <> 0 then end 88
#endif
print "PACKED_DESCRIPTORS_PASS"
end 0

' end of c-packed-descriptors.bas
