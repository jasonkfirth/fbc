/'
    Project: FreeBASIC C backend regression tests
    File: c-anonymous-header.bas
    Purpose: Exchange a naturally padded anonymous record with an ordinary C header.
    Responsibilities: Check by-value arguments and returns under mixed-declaration LTO.
    This file intentionally does NOT depend on compiler-owned padding members.
'/
#lang "fb"

type HeaderState
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

declare function header_accept cdecl alias "fb_test_header_accept"(byval state as HeaderState) as long
declare function header_make cdecl alias "fb_test_header_make"() as HeaderState

dim original as HeaderState = (7, 11, &h1234u, {13, 17})
if header_accept(original) = 0 then end 81
dim returned as HeaderState = header_make()
if returned.prefix <> 7 or returned.first_byte <> 11 then end 82
if returned.small_value <> &h1234u then end 83
if returned.trailing(0) <> 13 or returned.trailing(1) <> 17 then end 84
print "C_ANONYMOUS_HEADER_PASS"
end 0

' end of c-anonymous-header.bas
