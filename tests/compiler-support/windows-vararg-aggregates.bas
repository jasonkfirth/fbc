/'
    Project: FreeBASIC compiler regression tests
    File: windows-vararg-aggregates.bas
    Purpose: Check C variadic aggregate arguments at the Windows ABI boundary.
    Responsibilities: Pass one-, two-, four-, eight- and sixteen-byte records.
    This fixture does not implement the receiving C function.
'/

type Value1
    value as ubyte
end type

type Value2
    value as ushort
end type

type Value4
    value as ulong
end type

type Value8
    value as ulongint
end type

type Value16
    first_value as ulongint
    second_value as ulongint
end type

extern "C"
    declare function check_aggregate_arguments cdecl (byval marker as long, ...) as long
end extern

dim as Value1 a
dim as Value2 b
dim as Value4 c
dim as Value8 d
dim as Value16 e
a.value = &h11
b.value = &h2233
c.value = &h44556677ul
d.value = &h1122334455667788ull
e.first_value = &h1020304050607080ull
e.second_value = &h123456789abcdef0ull

if check_aggregate_arguments(17, a, b, c, d, e) <> -1 then end 1
print "WINDOWS_VARARG_AGGREGATES_PASS"

/' end of windows-vararg-aggregates.bas '/
