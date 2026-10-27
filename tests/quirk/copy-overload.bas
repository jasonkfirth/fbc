'' TEST_MODE : COMPILE_AND_RUN_OK
'' Project: FreeBASIC compiler qualification
'' File: copy-overload.bas
'' Purpose: Preserve existing mutable assignment overloads inside array fields.
'' Responsibilities: Check implicit record copies with a mutable element source.
'' This file intentionally does NOT mutate an actually const source object.
#lang "fb"
dim shared as integer copies
type mutable_item
	value as integer
	declare constructor()
	declare operator let( byref rhs as mutable_item )
end type
constructor mutable_item()
	value = 0
end constructor
operator mutable_item.let( byref rhs as mutable_item )
	copies += 1
	value = rhs.value
end operator
type mutable_bundle
	values(0 to 1) as mutable_item
end type
dim as mutable_bundle first_value, second_value
first_value.values(0).value = 17
first_value.values(1).value = 29
second_value = first_value
if( second_value.values(0).value <> 17 ) then end 1
if( second_value.values(1).value <> 29 or copies <> 2 ) then end 2

dim shared as integer mutable_copies, readonly_copies
type dual_item
	value as integer
	declare constructor()
	declare operator let( byref rhs as dual_item )
	declare operator let( byref rhs as const dual_item )
end type
constructor dual_item()
	value = 0
end constructor
operator dual_item.let( byref rhs as dual_item )
	mutable_copies += 1
	value = rhs.value
end operator
operator dual_item.let( byref rhs as const dual_item )
	readonly_copies += 1
	value = rhs.value
end operator
type dual_bundle
	values(0 to 1) as dual_item
end type
dim as dual_bundle first_dual, second_dual
first_dual.values(0).value = 31
first_dual.values(1).value = 43
second_dual = first_dual
if( second_dual.values(0).value <> 31 or second_dual.values(1).value <> 43 ) then end 3
if( mutable_copies <> 2 or readonly_copies <> 0 ) then end 4

sub copy_readonly( byref target as dual_item, byref source as const dual_item )
	target = source
end sub
copy_readonly(second_dual.values(0), first_dual.values(1))
if( second_dual.values(0).value <> 43 or readonly_copies <> 1 ) then end 5
print "MUTABLE_COPY_PASS"
end 0
'' end of copy-overload.bas
