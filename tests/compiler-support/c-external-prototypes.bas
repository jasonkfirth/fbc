/'
    Project: FreeBASIC C backend regressions
    File: c-external-prototypes.bas
    Purpose: Check external declarations against emitted procedure reachability.
    Responsibilities: Exercise dead cycles, public roots, callback initializers
    and multiple BASIC declarations of the same C storage.
    This file intentionally does NOT define the external C observer functions.
'/
#lang "fb"

#ifdef WIN_INTRINSIC_PROOF
    #include once "windows.bi"
#endif

extern "c"

'' Each pair binds the same C object. Reads, writes and addresses must
'' retain one storage identity when their repeated declarations are omitted.
extern first_scalar alias "prototype_shared_scalar" as long
extern first_values(0 to 2) alias "prototype_shared_array" as long
extern first_pointer alias "prototype_shared_pointer" as long ptr
extern second_scalar alias "prototype_shared_scalar" as long
extern second_values(0 to 2) alias "prototype_shared_array" as long
extern second_pointer alias "prototype_shared_pointer" as long ptr
extern second_initialized alias "prototype_defined_scalar" as long

extern initialized_scalar alias "prototype_defined_scalar" as long
dim shared initialized_scalar as long = 41

#ifdef CONFLICT_PROOF
'' An incompatible native declaration must still reach the C frontend.
extern incompatible_scalar alias "prototype_shared_scalar" as double
#endif

#ifdef CONFLICT_BOUNDS_PROOF
extern incompatible_values(0 to 3) alias "prototype_shared_array" as long
#endif

end extern

extern "c"

declare function prototype_dead(byval value as long) as long
declare function prototype_live(byval value as long) as long
declare function prototype_callback(byval value as long) as long
declare function prototype_public_target(byval value as long) as long
declare function prototype_call_public() as long
declare function prototype_dead_calls() as long
declare function prototype_shared_values() as long
declare function prototype_same_storage(byval left_address as const any ptr, _
    byval right_address as const any ptr) as long

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
    #include once "c-opaque-nop.bi"
    compiler_test_opaque_nop()
#endif

#ifdef CONFLICT_PROOF
if incompatible_scalar = 0 then end 8
#endif
#ifdef CONFLICT_BOUNDS_PROOF
if incompatible_values(3) = 0 then end 8
#endif
if live_helper(11) <> 21 then end 1
if callback_entry(12) <> 32 then end 2
if prototype_call_public() <> 43 then end 3
if prototype_dead_calls() <> 0 then end 4
if prototype_same_storage(@first_scalar, @second_scalar) = 0 then end 9
if prototype_same_storage(@first_values(0), @second_values(0)) = 0 then end 10
if prototype_same_storage(@first_pointer, @second_pointer) = 0 then end 11
if first_scalar <> 17 or second_values(1) <> 29 then end 12
if prototype_same_storage(@initialized_scalar, @second_initialized) = 0 then end 15
if second_initialized <> 41 then end 16
first_scalar = 37
second_values(1) = 43
second_initialized = 53
if first_pointer <> @first_values(1) then end 13
*second_pointer += 5
if prototype_shared_values() <> 138 then end 14
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
