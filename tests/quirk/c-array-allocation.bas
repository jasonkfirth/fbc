'' TEST_MODE : COMPILE_AND_RUN_OK
'' Project: FreeBASIC compiler regression tests
'' File: c-array-allocation.bas
'' Purpose: Verify ordinary REDIM initialization through the C runtime.
'' Responsibilities: Check fixed-string spaces, zero filling and constructors.
'' This file intentionally does NOT inject allocator failures or test array bias.
#lang "fb"

dim shared as integer constructor_calls

type allocated_item
	stamp as integer
	declare constructor()
end type

constructor allocated_item()
	constructor_calls += 1
	stamp = &h5a
end constructor

dim texts() as string * 8
redim texts(0 to 2)
for i as integer = 0 to 2
	if( texts(i) <> space(8) ) then end 1
next
texts(1) = "changed"
redim texts(0 to 2)
for i as integer = 0 to 2
	if( texts(i) <> space(8) ) then end 2
next

dim zeroed() as longint
redim zeroed(0 to 2)
for i as integer = 0 to 2
	if( zeroed(i) <> 0 ) then end 3
next

dim items() as allocated_item
redim items(0 to 2)
if( constructor_calls <> 3 ) then end 4
for i as integer = 0 to 2
	if( items(i).stamp <> &h5a ) then end 5
next
redim items(0 to 0)
if( constructor_calls <> 4 ) then end 6
if( items(0).stamp <> &h5a ) then end 7

print "c-array-allocation-ok"
'' end of c-array-allocation.bas
