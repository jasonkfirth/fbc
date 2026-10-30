/'
	Project: FreeBASIC C backend regression tests
	File: c-unused-scalar-stores.bas
	Purpose: Verify unobserved scalar stores can disappear without losing calls.
	Responsibilities: Exercise constants, integer coercions, returned pointers,
	floating results, address escapes, function results and persistent storage.
	This file intentionally does NOT rely on native warning suppression.
'/
#lang "fb"

dim shared as long effects
dim shared as long call_order

dim shared as long pointed_value = 31

private function touch(byval amount as long) as long
	effects += amount
	return amount
end function

private function touch_single() as single
	effects += 5
	return 1.25
end function

private function touch_pointer() as long ptr
	effects += 7
	return @pointed_value
end function

private sub change_value(byref value as long)
	if value <> 23 then end 1
	value += 7
	effects += 13
end sub

private function ordered(byval digit as long) as long
	call_order = call_order * 10 + digit
	return digit
end function

private function pointer_with_effect() as long ptr
	effects += 17
	return @pointed_value
end function

private sub discard_values()
	dim dead_constant as integer = 7
	dead_constant = 42
	dim default_value as double
	dim ignored_result as integer = touch(2)
	ignored_result = touch(3)
	dim ignored_float as single = touch_single()
	dim ignored_pointer as long ptr = touch_pointer()
	dim ignored_byte as ubyte = cubyte(touch(257))
	dim ignored_calculation as long = ordered(1) + ordered(2) * ordered(3)
	dim ignored_compare as integer = (ordered(4) < ordered(5))
	dim ignored_load as long = *pointer_with_effect()
	dim dead_left as long = 7
	dim dead_right as long = dead_left * 9 + 12
end sub

private function read_argument(byref value as long) as long
	value += 1
	return value
end function

private function checked_result() as long
	checked_result = 91
end function

private function persistent_result() as long
	static as long calls = 1
	calls += 1
	return calls
end function

private sub observed_chain()
	dim seed_value as long = touch(19)
	dim middle_value as long = seed_value + 2
	dim final_value as long = middle_value * 3
	if final_value <> 63 then end 14
end sub

private sub observed_values()
	dim actual as long = touch(11)
	if actual <> 11 then end 2
	dim escaped as long = 23
	change_value(escaped)
	if escaped <> 30 then end 3
	if checked_result() <> 91 then end 4
	if persistent_result() <> 2 then end 5
	if persistent_result() <> 3 then end 6
end sub

discard_values()
if effects <> 291 then end 7
if call_order <> 12345 then end 13
observed_values()
if effects <> 315 then end 8
if pointed_value <> 31 then end 9
if read_argument(37) <> 38 then end 10
if read_argument(touch(17)) <> 18 then end 11
if effects <> 332 then end 12
observed_chain()
if effects <> 351 then end 15
print "C_UNUSED_SCALAR_STORES_PASS"

'' end of c-unused-scalar-stores.bas
