/'
    Project: FreeBASIC C backend regression tests
    File: c-operator-reachability.bas
    Purpose: Check reachability of private overloaded division operators.
    Responsibilities: Exercise live calls, dead overloads and transitive calls.
    This fixture intentionally does NOT depend on opaque assembly.
'/
#lang "fb"

type Number
    value as long
end type

private operator / (byref lhs as const Number, byref rhs as const Number) as Number
    dim as Number result
    result.value = lhs.value \ rhs.value
    return result
end operator

private operator / (byref lhs as const Number, byval rhs as long) as Number
    print "UNUSED_PRIVATE_DIVISION_MARKER"
    dim as Number result
    result.value = lhs.value \ rhs
    return result
end operator

private function calculate(byref lhs as const Number, byref rhs as const Number) as Number
    return lhs / rhs
end function

function used_result(byref lhs as const Number, byref rhs as const Number) as long
    dim as Number quotient = calculate(lhs, rhs)
    return quotient.value
end function

private function never_called(byref lhs as const Number) as Number
    return lhs / 2
end function

dim as Number lhs, rhs
lhs.value = 42
rhs.value = 7
if used_result(lhs, rhs) <> 6 then end 1
print "C_DIVISION_REACHABILITY_PASS"

/' end of c-operator-reachability.bas '/
