/'
    Project: FreeBASIC C backend regression tests
    File: c23-parameters-asm.bas
    Purpose: Preserve parameter names when opaque assembly is present.
    Responsibilities: Check the conservative assembly fallback.
    This fixture does not depend on assembler side effects.
'/
#lang "fb"

function asm_case cdecl(byval used as long, byval ignored as long) as long
    asm
        nop
    end asm
    return used
end function

if asm_case(4, 5) <> 4 then end 1
print "C23_PARAMETER_ASM_PASS"

/' end of c23-parameters-asm.bas '/
