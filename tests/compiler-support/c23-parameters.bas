/'
    Project: FreeBASIC C backend regression tests
    File: c23-parameters.bas
    Purpose: Exercise unused parameter names in C23 and GNU11 definitions.
    Responsibilities: Check callbacks, argument evaluation and managed values.
    This fixture does not depend on the generated C parameter spelling.
'/
#lang "fb"

dim shared as long evaluations, destroyed

type Managed
    value as long
    declare constructor(byval n as long)
    declare destructor()
end type

constructor Managed(byval n as long)
    value = n
end constructor

destructor Managed()
    destroyed += 1
end destructor

function next_value() as long
    evaluations += 1
    return evaluations
end function

function scalar_case cdecl(byval used as long, byval ignored as long) as long
    return used
end function

function pointer_case cdecl(byref ignored as long, byval value as long) as long
    return value
end function

function callback_case cdecl(byval value as long, byval ignored as long) as long
    return value + 2
end function

function string_case cdecl(byval value as string, byval ignored as long) as string
    return value
end function

function managed_case cdecl(byref value as Managed, byval ignored as long) as long
    return value.value
end function

function unused_string_case cdecl(byval ignored as string) as long
    return 23
end function

function vararg_case cdecl(byval value as long, byval ignored as long, ...) as long
    return value
end function

function vararg_read cdecl(byval marker as long, byval count as long, ...) as long
    dim as cva_list args
    cva_start(args, count)
    dim as long value = cva_arg(args, long)
    cva_end(args)
    return marker + count + value
end function

dim as long arg = 8
dim as function cdecl(byval as long, byval as long) as long callback = @callback_case
if scalar_case(7, next_value()) <> 7 then end 1
if evaluations <> 1 then end 2
if pointer_case(arg, next_value()) <> 2 then end 3
if callback(3, next_value()) <> 5 then end 4
if string_case("hello", next_value()) <> "hello" then end 5
if unused_string_case("managed argument") <> 23 then end 8
if vararg_case(29, next_value(), 31) <> 29 then end 9
if vararg_read(1, 2, 3) <> 6 then end 10
scope
    dim as Managed value = Managed(19)
    if managed_case(value, next_value()) <> 19 then end 6
end scope
if evaluations <> 6 orelse destroyed <> 1 then end 7
print "C23_PARAMETER_PASS"

/' end of c23-parameters.bas '/
