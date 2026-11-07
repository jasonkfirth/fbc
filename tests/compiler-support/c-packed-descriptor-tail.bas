/'
    Project: FreeBASIC C backend regression tests
    File: c-packed-descriptor-tail.bas
    Purpose: Preserve trailing bytes and array strides in packed managed records.
    Responsibilities: Check descriptor offsets, nested storage and ordinary lifetime.
    This file intentionally does NOT use ERROR handlers or partial construction.
'/
#lang "fb"
#ifndef TEST_PACKING
    #define TEST_PACKING 1
#endif

type StringTail field = TEST_PACKING
    prefix as ubyte
    text as string
    suffix as ubyte
end type

type ArrayTail field = TEST_PACKING
    prefix as ubyte
    values(any) as integer
    suffix as ubyte
end type

type TailState field = TEST_PACKING
    prefix as ubyte
    text as string
    words(0 to 1) as string
    values(any) as integer
    suffix as ubyte
end type

type ByteWrapper field = 1
    prefix as ubyte
    child as TailState
    suffix as ubyte
end type

type NaturalWrapper
    prefix as ubyte
    child as TailState
    suffix as ubyte
end type

'' Descriptors retain natural member offsets, while FIELD=N controls the
'' enclosing record's trailing size. Consecutive array elements can therefore
'' contain descriptors whose actual addresses are below natural alignment.
#if TEST_PACKING < sizeof(any ptr)
    const effective_packing = TEST_PACKING
#else
    const effective_packing = sizeof(any ptr)
#endif
const tail_size = ((19 * sizeof(any ptr) + effective_packing) \ effective_packing) * effective_packing
#assert sizeof(StringTail) = ((4 * sizeof(any ptr) + effective_packing) \ effective_packing) * effective_packing
#assert sizeof(ArrayTail) = ((10 * sizeof(any ptr) + effective_packing) \ effective_packing) * effective_packing
#assert sizeof(TailState) = tail_size
#assert sizeof(ByteWrapper) = tail_size + 2

private sub fill_state(byref state as TailState)
    state.prefix = 17
    state.suffix = 23
    state.text = "packed tail"
    state.words(1) = "fixed field"
    redim state.values(0 to 1)
    state.values(1) = 41
end sub

private function check_state(byval state as TailState) as integer
    if state.prefix <> 17 or state.suffix <> 23 then return 0
    if state.text <> "packed tail" then return 0
    if state.words(1) <> "fixed field" then return 0
    if state.values(1) <> 41 then return 0
    return -1
end function

private sub fill_array(byref state as ArrayTail)
    redim state.values(0 to 1)
    state.values(1) = 43
    state.suffix = 31
end sub

private sub exercise()
    dim strings(0 to 1) as StringTail
    dim arrays(0 to 1) as ArrayTail
    dim states(0 to 1) as TailState
    dim unaligned as ByteWrapper
    dim natural as NaturalWrapper
    dim dynamic_states(any) as TailState

    if cuint(@states(1)) - cuint(@states(0)) <> tail_size then end 81
    if cuint(@unaligned.child) - cuint(@unaligned) <> 1 then end 82
    if cuint(@natural.child) - cuint(@natural) <> sizeof(any ptr) then end 83
    if cuint(@states(0).text) - cuint(@states(0)) <> sizeof(any ptr) then end 84

    strings(1).text = "trailing string"
    strings(1).suffix = 29
    fill_array(arrays(1))
    if strings(1).text <> "trailing string" or strings(1).suffix <> 29 then end 85
    if arrays(1).values(1) <> 43 or arrays(1).suffix <> 31 then end 86

    fill_state(states(1))
    fill_state(unaligned.child)
    fill_state(natural.child)
    redim dynamic_states(0 to 1)
    fill_state(dynamic_states(1))
    if check_state(states(1)) = 0 then end 87
    if check_state(unaligned.child) = 0 then end 88
    if check_state(natural.child) = 0 then end 89
    if check_state(dynamic_states(1)) = 0 then end 90
end sub

exercise()
#ifdef TEST_ALIGNMENT
    '' Query after every ordinary local destructor has released its fields.
    declare function descriptor_alignment_errors cdecl alias "descriptor_alignment_errors"() as ulong
    if descriptor_alignment_errors() <> 0 then end 91
#endif
print "PACKED_DESCRIPTOR_TAIL_PASS"
end 0

' end of c-packed-descriptor-tail.bas
