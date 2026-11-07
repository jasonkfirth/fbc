/'
    Project: FreeBASIC C backend qualification
    File: c-array-virtual-origins.bas
    Purpose: Check descriptor origins across array storage owners and resizing.
    Responsibilities: Preserve record fields, element values and index evaluation.
    This file intentionally does NOT allocate large buffers or use invalid indices.
'/
#lang "fb"

#ifdef NEGATIVE_BOUNDS
    #ifdef __FB_64BIT__
        const EDGE_BOUND as integer = -9223372036854775807ll - 1ll
    #else
        const EDGE_BOUND as integer = -2147483647 - 1
    #endif
#else
#ifdef __FB_64BIT__
    const EDGE_BOUND as integer = 9223372036854775807ll - 3ll
#else
    const EDGE_BOUND as integer = 2147483647 - 3
#endif
#endif

const FIRST_VALUE as ubyte = 17
const LAST_VALUE as ubyte = 25
const GROWN_VALUE as ubyte = 31
const EXPECTED_TOTAL as integer = FIRST_VALUE + LAST_VALUE
const EXPECTED_GROWN_TOTAL as integer = FIRST_VALUE + GROWN_VALUE
const OWNER_SENTINEL as integer = 123

type Triple
    first as ubyte
    middle as ubyte
    last as ubyte
end type

type Owner
    sentinel as integer
    entries(any) as Triple
end type

dim shared byte_values() as ubyte
dim shared global_values() as Triple
dim shared calls as integer

private function next_index(byval offset as integer) as integer
    calls += 1
    return EDGE_BOUND + offset
end function

private function sum_ends(values() as Triple) as integer
    return values(lbound(values)).first + values(ubound(values)).last
end function

private sub check_bytes()
    redim byte_values(EDGE_BOUND to EDGE_BOUND + 1)
    byte_values(EDGE_BOUND) = FIRST_VALUE
    byte_values(EDGE_BOUND + 1) = LAST_VALUE
    if byte_values(EDGE_BOUND) + byte_values(EDGE_BOUND + 1) <> EXPECTED_TOTAL then end 7
    erase byte_values
end sub

private sub check_global()
    redim global_values(EDGE_BOUND to EDGE_BOUND + 1)
    calls = 0
    global_values(next_index(0)).first = FIRST_VALUE
    global_values(next_index(1)).last = LAST_VALUE
    if calls <> 2 or sum_ends(global_values()) <> EXPECTED_TOTAL then end 1
    redim preserve global_values(EDGE_BOUND to EDGE_BOUND + 2)
    global_values(EDGE_BOUND + 2).last = GROWN_VALUE
    if sum_ends(global_values()) <> EXPECTED_GROWN_TOTAL then end 2
    erase global_values
end sub

private sub check_local()
    dim values() as Triple
    redim values(EDGE_BOUND to EDGE_BOUND + 1)
    calls = 0
    values(next_index(0)).first = FIRST_VALUE
    values(next_index(1)).last = LAST_VALUE
    if calls <> 2 or sum_ends(values()) <> EXPECTED_TOTAL then end 3
    redim preserve values(EDGE_BOUND to EDGE_BOUND + 2)
    values(EDGE_BOUND + 2).last = GROWN_VALUE
    if sum_ends(values()) <> EXPECTED_GROWN_TOTAL then end 4
end sub

private sub check_member()
    dim value as Owner
    value.sentinel = OWNER_SENTINEL
    redim value.entries(EDGE_BOUND to EDGE_BOUND + 1)
    calls = 0
    value.entries(next_index(0)).first = FIRST_VALUE
    value.entries(next_index(1)).last = LAST_VALUE
    if calls <> 2 or sum_ends(value.entries()) <> EXPECTED_TOTAL then end 5
    redim preserve value.entries(EDGE_BOUND to EDGE_BOUND + 2)
    value.entries(EDGE_BOUND + 2).last = GROWN_VALUE
    if sum_ends(value.entries()) <> EXPECTED_GROWN_TOTAL or value.sentinel <> OWNER_SENTINEL then end 6
end sub

check_bytes()
check_global()
check_local()
check_member()
print "ARRAY_ORIGIN=PASS"
end 0
' end of c-array-virtual-origins.bas
