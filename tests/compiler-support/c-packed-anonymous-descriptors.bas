/'
    Project: FreeBASIC C backend regression tests
    File: c-packed-anonymous-descriptors.bas
    Purpose: Preserve anonymous container offsets inside managed records.
    Responsibilities: Check inherited and explicit packing, copies and array lifetime.
    This file intentionally does NOT place managed fields inside a union.
'/
#lang "fb"
#ifndef TEST_PACKING
    #define TEST_PACKING 1
#endif

type AnonymousState field = TEST_PACKING
    prefix as ubyte
    union
        sequence as uinteger
        type
            left_word as ushort
            right_byte as ubyte
        end type
    end union
    text as string
    words(0 to 1) as string
    values(any) as integer
    suffix as ubyte
end type

type ExplicitState field = TEST_PACKING
    prefix as ubyte
    union field = 4
        type field = 2
            start_byte as ubyte
            payload as uinteger
            end_byte as ubyte
        end type
        raw_bytes(0 to 11) as ubyte
    end union
    text as string
    values(any) as integer
    union field = 2
        trailer_word as ushort
        type field = 1
            flags as ubyte
            small_word as ushort
        end type
    end union
    suffix as ubyte
end type

type ByteWrapper field = 1
    prefix as ubyte
    child as AnonymousState
    suffix as ubyte
end type

type TrivialState field = TEST_PACKING
    prefix as ubyte
    union field = 2
        type field = 1
            first_byte as ubyte
            small_value as ushort
        end type
        raw_values(0 to 2) as ubyte
    end union
    trailing(0 to 1) as ubyte
end type

union ExplicitChoice
    type field = 1
        code as ubyte
        value as ulong
        tail as ubyte
    end type
end union

dim shared initialized as TrivialState = (7, 11, &h1234u, {13, 17})
dim shared initialized_choice as ExplicitChoice = (53, &h11223344ul, 59)

private sub fill_anonymous(byref state as AnonymousState)
    state.prefix = 17
    state.sequence = &h12345678u
    state.text = "anonymous tail"
    state.words(1) = "fixed field"
    redim state.values(0 to 1)
    state.values(1) = 41
    state.suffix = 23
end sub

private function check_anonymous(byval state as AnonymousState) as integer
    if state.prefix <> 17 or state.suffix <> 23 then return 0
    if state.sequence <> &h12345678u then return 0
    if state.text <> "anonymous tail" or state.words(1) <> "fixed field" then return 0
    if state.values(1) <> 41 then return 0
    return -1
end function

private sub fill_explicit(byref state as ExplicitState)
    state.prefix = 29
    state.start_byte = 31
    state.payload = 1234567
    state.end_byte = 37
    state.text = "explicit packing"
    redim state.values(0 to 1)
    state.values(1) = 43
    state.trailer_word = &h1234u
    state.suffix = 47
end sub

private function check_explicit(byval state as ExplicitState) as integer
    if state.prefix <> 29 or state.suffix <> 47 then return 0
    if state.start_byte <> 31 or state.end_byte <> 37 then return 0
    if state.payload <> 1234567 then return 0
    if state.text <> "explicit packing" or state.values(1) <> 43 then return 0
    if state.trailer_word <> &h1234u then return 0
    return -1
end function

private function check_trivial(byval state as TrivialState) as integer
    if state.prefix <> 7 or state.first_byte <> 11 then return 0
    if state.small_value <> &h1234u then return 0
    if state.trailing(0) <> 13 or state.trailing(1) <> 17 then return 0
    return -1
end function

private function check_choice(byval state as ExplicitChoice) as integer
    if state.code <> 53 or state.tail <> 59 then return 0
    if state.value <> &h11223344ul then return 0
    return -1
end function

private sub exercise()
    dim inherited(0 to 1) as AnonymousState
    dim explicit_records(0 to 1) as ExplicitState
    dim wrapped as ByteWrapper
    dim dynamic_records(any) as ExplicitState

    if cuint(@inherited(1)) - cuint(@inherited(0)) <> sizeof(AnonymousState) then end 81
    if cuint(@explicit_records(1)) - cuint(@explicit_records(0)) <> sizeof(ExplicitState) then end 82
    if cuint(@wrapped.child) - cuint(@wrapped) <> 1 then end 83
    if cuint(@explicit_records(0).payload) - cuint(@explicit_records(0).start_byte) <> 2 then end 84

    fill_anonymous(inherited(1))
    fill_anonymous(wrapped.child)
    fill_explicit(explicit_records(1))
    redim dynamic_records(0 to 1)
    fill_explicit(dynamic_records(1))
    if check_anonymous(inherited(1)) = 0 then end 85
    if check_anonymous(wrapped.child) = 0 then end 86
    if check_explicit(explicit_records(1)) = 0 then end 87
    if check_explicit(dynamic_records(1)) = 0 then end 88
    if check_trivial(initialized) = 0 then end 90
    if check_choice(initialized_choice) = 0 then end 91
end sub

exercise()
#ifdef TEST_ALIGNMENT
    declare function descriptor_alignment_errors cdecl alias "descriptor_alignment_errors"() as ulong
    if descriptor_alignment_errors() <> 0 then end 89
#endif
print "PACKED_ANONYMOUS_DESCRIPTORS_PASS"
end 0

' end of c-packed-anonymous-descriptors.bas
