/'
    Project: FreeBASIC C backend regression tests
    File: c-opaque-local-metadata.bas
    Purpose: Keep assembly storage while pruning unrelated private C metadata.
    Responsibilities: Static descriptors, required BYDESC metadata, parameter ABI
    and caller-side evaluation across procedures with opaque assembly.
    This file intentionally does NOT depend on stack offsets or raw C names.
'/
#lang "fb"

dim shared as long calls
private function nextValue() as long
    calls += 1
    return 9
end function

private function ignoredArgument(byval value as long, byval ignored as long) as long
    return value
end function

private function descriptorValue(values() as long) as long
    return lbound(values) + ubound(values) + values(lbound(values))
end function

private function arrayValues(byval value as long) as long
    static first(0 to 2) as long
    static second(0 to 3) as long
    static described(2 to 4) as long
    first(0) = value
    second(1) = value + 1
    described(2) = value + 2
#ifdef __FB_X86__
#if __FB_ASM__ = "intel"
    '' ADD retains the general assembly boundary; it is not the narrow INC/DEC
    '' form accepted by scalar dead-store analysis. The symbol operand stays live.
    asm
        add dword ptr [value], 2
    end asm
#endif
#endif
    return first(0) + second(1) + descriptorValue(described()) + value
end function

dim as long result = arrayValues(10)
#ifdef __FB_X86__
#if __FB_ASM__ = "intel"
if result <> 51 then end 1
#else
if result <> 49 then end 1
#endif
#else
if result <> 49 then end 1
#endif
if ignoredArgument(7, nextValue()) <> 7 then end 2
if calls <> 1 then end 3
print "C_OPAQUE_LOCAL_METADATA_PASS"

/' end of c-opaque-local-metadata.bas '/
