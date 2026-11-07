/'
    Project: FreeBASIC backend regression tests
    File: procedure-address-identity.bas
    Purpose: Compare procedure addresses by their native machine values.
    Responsibilities: Check aliases, casts, stored pointers, evaluation counts,
    unsigned pointer ordering and GAS64 symbol-address operands.
    This file intentionally does NOT call an invented pointer value.
'/
#lang "fb"

type Callback as function cdecl(byval as long) as long
extern "C"
    declare function FirstEntry cdecl alias "fb_alias_target"(byval value as long) as long
    declare function SecondEntry cdecl alias "fb_alias_target"(byval value as long) as long
    declare function OtherEntry cdecl alias "fb_alias_other"(byval value as long) as long
    declare function ReferenceEntry cdecl alias "fb_alias_reference"() as Callback
    declare function ReferenceData cdecl alias "fb_alias_data_reference"() as long ptr
    extern first_data alias "fb_alias_data" as long
    extern second_data alias "fb_alias_data" as long
end extern

dim shared as long evaluations
private function NextAddress() as Callback
    evaluations += 1
    return @FirstEntry
end function

if( @FirstEntry <> @SecondEntry ) then end __LINE__
if( not(@FirstEntry = @SecondEntry) ) then end __LINE__
if( @FirstEntry = @OtherEntry ) then end __LINE__
if( @FirstEntry <> ReferenceEntry() ) then end __LINE__
if( FirstEntry(10) <> 17 or SecondEntry(10) <> 17 or OtherEntry(10) <> 7 ) then end __LINE__

dim as integer equal_value = (@FirstEntry = @SecondEntry)
dim as integer unequal_value = (@FirstEntry <> @SecondEntry)
dim as boolean boolean_value = (@FirstEntry = @SecondEntry)
if( equal_value <> -1 or unequal_value <> 0 ) then end __LINE__
if( boolean_value <> true ) then end __LINE__

dim as Callback first_pointer = @FirstEntry
dim as Callback second_pointer = @SecondEntry
dim as Callback null_pointer = 0
if( @FirstEntry <> second_pointer or first_pointer <> @SecondEntry ) then end __LINE__
if( first_pointer <> second_pointer ) then end __LINE__
if( first_pointer = null_pointer or null_pointer = first_pointer ) then end __LINE__
if( null_pointer <> 0 or 0 <> null_pointer ) then end __LINE__
if( cptr(any ptr, @FirstEntry) <> cptr(any ptr, @SecondEntry) ) then end __LINE__
dim as any ptr erased_first = cptr(any ptr, @FirstEntry)
dim as any ptr erased_second = cptr(any ptr, @SecondEntry)
if( erased_first <> erased_second ) then end __LINE__

if( NextAddress() <> NextAddress() ) then end __LINE__
if( evaluations <> 2 ) then end __LINE__
if( cuint(@FirstEntry) <> cuint(ReferenceEntry()) ) then end __LINE__
if( cuint(@FirstEntry) - cuint(@SecondEntry) <> 0 ) then end __LINE__

'' Relational pointer operators use unsigned addresses. Explicit signed
'' integer casts retain the requested signed arithmetic instead.
dim as Callback high_pointer = cast(Callback, cuint(-1))
if( (high_pointer < first_pointer) <> (cuint(high_pointer) < cuint(first_pointer)) ) then end __LINE__
if( (high_pointer <= first_pointer) <> (cuint(high_pointer) <= cuint(first_pointer)) ) then end __LINE__
if( (high_pointer > first_pointer) <> (cuint(high_pointer) > cuint(first_pointer)) ) then end __LINE__
if( (high_pointer >= first_pointer) <> (cuint(high_pointer) >= cuint(first_pointer)) ) then end __LINE__
if( cint(high_pointer) >= 0 ) then end __LINE__

'' Data symbols also use offset operands in the assembler backend. Their
'' addresses must be compared without reading the data as an address.
if( @first_data <> @second_data ) then end __LINE__
if( @first_data <> ReferenceData() ) then end __LINE__

/' end of procedure-address-identity.bas '/
