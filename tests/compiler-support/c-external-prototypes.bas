/'
    Project: FreeBASIC C backend regressions
    File: c-external-prototypes.bas
    Purpose: Check external declarations against emitted procedure reachability.
    Responsibilities: Exercise dead cycles, public roots and callback initializers.
    This file intentionally does NOT define the external C observer functions.
'/
#lang "fb"

#ifdef WIN_INTRINSIC_PROOF
    #include once "windows.bi"
#endif

extern "c"

declare function prototype_dead(byval value as long) as long
declare function prototype_live(byval value as long) as long
declare function prototype_callback(byval value as long) as long
declare function prototype_public_target(byval value as long) as long
declare function prototype_call_public() as long
declare function prototype_dead_calls() as long

function prototype_public_entry(byval value as long) as long
    return prototype_public_target(value)
end function

end extern

private declare function dead_cycle_a(byval value as long) as long

private function dead_cycle_b(byval value as long) as long
    if value > 0 then return dead_cycle_a(value - 1)
    return prototype_dead(value)
end function

private function dead_cycle_a(byval value as long) as long
    return dead_cycle_b(value)
end function

private function live_helper(byval value as long) as long
    return prototype_live(value)
end function

'' A global function address is a root even without a direct C call.
dim shared callback_entry as function cdecl(byval as long) as long = @prototype_callback

#ifdef OPAQUE_PROOF
    '' Inline assembly prevents this output from proving its complete call graph.
    asm
        nop
    end asm
#endif

if live_helper(11) <> 21 then end 1
if callback_entry(12) <> 32 then end 2
if prototype_call_public() <> 43 then end 3
if prototype_dead_calls() <> 0 then end 4
#ifdef WIN_INTRINSIC_PROOF
    '' These Windows wrappers require the external intrinsic declarations.
    dim as long counter32 = 7
    dim as longint counter64 = 19
    if InterlockedAdd(@counter32, 3) <> 10 then end 5
    if InterlockedAdd64(@counter64, 5) <> 24 then end 6
    if counter32 <> 10 or counter64 <> 24 then end 7
#endif
print "EXTERNAL_PROTOTYPES_PASS"

' end of c-external-prototypes.bas
