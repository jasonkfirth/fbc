'' TEST_MODE : COMPILE_AND_RUN_OK
'' Project: FreeBASIC compiler regression tests
'' File: c-defined-calls.bas
'' Purpose: Verify call operands, special headers and temporary AST ownership.
'' Responsibilities: Cover callbacks, nested arguments, record results,
''     unnamed prototypes, constructors, operators and properties.
'' This file intentionally does NOT depend on an allocator's released-node data.
#lang "fb"

type pair_value
	left_value as long
	right_value as long
end type
declare function sum_pair( byref as const pair_value ) as long
function sum_pair( byref value as const pair_value ) as long
	return value.left_value + value.right_value
end function
function make_pair( byval left_value as long, byval right_value as long ) as pair_value
	dim as pair_value result = (left_value, right_value)
	return result
end function

dim shared as long effect_count, constructed, destroyed
function effect( byval value as long ) as long
	effect_count += 1
	return value
end function
function combine cdecl( byval left_value as long, byval right_value as long ) as long
	return left_value * 100 + right_value
end function
type callback as function cdecl( byval as long, byval as long ) as long
dim as callback targets(0 to 1) = { @combine, @combine }

type tracked_value
	value as long
	declare constructor()
	declare destructor()
	declare operator cast() as long
	declare property current_value() as long
end type
constructor tracked_value()
	constructed += 1
	value = 23
end constructor
destructor tracked_value()
	destroyed += 1
end destructor
operator tracked_value.cast() as long
	return value
end operator
property tracked_value.current_value() as long
	return value
end property

'' The target expression and both arguments each run once.
if( targets(effect(1))(effect(7), effect(22)) <> 722 ) then end 1
if( effect_count <> 3 ) then end 2
dim as pair_value pair = make_pair(effect(4), effect(5))
if( sum_pair(pair) <> 9 or effect_count <> 5 ) then end 3
scope
	dim as tracked_value item
	if( clng(item) <> 23 or item.current_value <> 23 ) then end 4
end scope
if( constructed <> 1 or destroyed <> 1 ) then end 5

'' Keep register-argument cases in the same source for Win32 object qualification.
#if defined(__FB_WIN32__) andalso not defined(__FB_64BIT__)
function fast_combine __fastcall( byval left_value as long, byval right_value as long ) as long
	return left_value * 100 + right_value
end function
type fast_callback as function __fastcall( byval as long, byval as long ) as long
dim as fast_callback fast_target = @fast_combine
if( fast_target(effect(7), effect(22)) <> 722 ) then end 6
#endif
print "DEFINED_CALLS_PASS"
end 0
'' end of c-defined-calls.bas
