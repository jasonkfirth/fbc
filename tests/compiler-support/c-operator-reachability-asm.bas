/'
    Project: FreeBASIC C backend regression tests
    File: c-operator-reachability-asm.bas
    Purpose: Check conservative division-operator handling around inline assembly.
    Responsibilities: Keep an otherwise unreachable operator body available.
    This fixture intentionally does NOT rely on assembler side effects.
'/
#lang "fb"

type Number
    value as long
end type

private operator / (byref lhs as const Number, byval rhs as long) as Number
    print "ASM_FALLBACK_DIVISION_MARKER"
    dim as Number result
    result.value = lhs.value \ rhs
    return result
end operator

private function never_called(byref value as const Number) as Number
    return value / 2
end function

asm
    nop
end asm
print "C_DIVISION_ASM_FALLBACK_PASS"

/' end of c-operator-reachability-asm.bas '/
