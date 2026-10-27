'' TEST_MODE : COMPILE_ONLY_OK
'' Project: FreeBASIC compiler regression reductions
'' File: c-array-copy-object-failure.bas
'' Purpose: Fail the object-array copy allocator after releasing old elements.
'' Responsibilities: Preserve errors and element lifetimes across repeated copies.
'' This file intentionally does NOT replace allocation outside REDIMTO.
#lang "fb"
#include once "fberror.bi"

declare function AllocationFailures cdecl alias "fb_CopyAllocationFailures" () as long
dim shared as integer live_items
dim shared as integer copied_items

type item
	value as integer
	declare constructor()
	declare destructor()
	declare operator let( byref rhs as const item )
end type

constructor item()
	value = 0
	live_items += 1
end constructor

destructor item()
	live_items -= 1
end destructor

operator item.let( byref rhs as const item )
	value = rhs.value
	copied_items += 1
end operator

type holder
	leading as integer
	values(any) as item
	trailing as integer
end type

scope
	dim as holder source_value, destination
	source_value.leading = 17
	source_value.trailing = 23
	redim source_value.values(0 to 1)
	source_value.values(0).value = 101
	source_value.values(1).value = 102
	redim destination.values(0 to 0)
	if live_items <> 3 then end 1

	destination = source_value
	if err <> cint(FB.FB_RTERROR_OUTOFMEM) then end 2
	if AllocationFailures() <> 1 then end 3
	if ubound(destination.values) <> -1 then end 4
	if destination.leading <> 17 or destination.trailing <> 23 then end 5
	if live_items <> 2 or copied_items <> 0 then end 6
	if source_value.values(0).value <> 101 or source_value.values(1).value <> 102 then end 7

	'' Reusing the failed destination must release old elements without
	'' allocating replacement storage or constructing/copying any items.
	dim as holder repeated
	repeated.leading = -1
	repeated.trailing = -1
	redim repeated.values(4 to 4)
	if live_items <> 3 then end 9
	repeated = destination
	if AllocationFailures() <> 1 then end 10
	if err <> 0 then end 11
	if ubound(repeated.values) <> -1 then end 12
	if repeated.leading <> 17 or repeated.trailing <> 23 then end 13
	if live_items <> 2 or copied_items <> 0 then end 14
end scope

if live_items <> 0 then end 8
end 0
'' end of c-array-copy-object-failure.bas
