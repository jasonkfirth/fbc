'' Project: FreeBASIC null pointer ordering qualification
'' File: c-pointer-null-order.bas
'' Purpose: Keep operand evaluation when unsigned null ordering is constant.
'' Responsibilities: Cover both operand positions, BASIC truth values and branches.
'' This file intentionally does NOT dereference constructed addresses.
#lang "fb"

dim shared visits as integer

function operand(byval value as any ptr) as any ptr
	visits += 1
	return value
end function

sub check_null(byval value as any ptr)
	dim results(0 to 7) as integer
	dim expected(0 to 7) as integer
	expected(0) = cuint(value) > 0
	expected(1) = cuint(value) < 0
	expected(2) = cuint(value) >= 0
	expected(3) = cuint(value) <= 0
	expected(4) = 0 > cuint(value)
	expected(5) = 0 < cuint(value)
	expected(6) = 0 >= cuint(value)
	expected(7) = 0 <= cuint(value)
	visits = 0
	results(0) = operand(value) > 0
	results(1) = operand(value) < 0
	results(2) = operand(value) >= 0
	results(3) = operand(value) <= 0
	results(4) = 0 > operand(value)
	results(5) = 0 < operand(value)
	results(6) = 0 >= operand(value)
	results(7) = 0 <= operand(value)
	if visits <> 8 then end 31
	for i as integer = 0 to 7
		if results(i) <> expected(i) then end 32 + i
	next
	visits = 0
	if operand(value) >= 0 then
		if visits <> 1 then end 41
	else
		end 42
	end if
	if operand(value) < 0 then end 43
	if visits <> 2 then end 44
	if not (0 <= operand(value)) then end 45
	if visits <> 3 then end 46
	if (operand(value) >= 0) andalso (operand(value) <= 0) then
		if value <> 0 then end 47
	else
		if value = 0 then end 48
	end if
	if visits <> 5 then end 49
end sub

check_null(0)
check_null(cptr(any ptr, 1))
check_null(cptr(any ptr, not cuint(0)))
check_null(cptr(any ptr, cuint(1) shl (sizeof(any ptr) * 8 - 1)))
print "POINTER_NULL_ORDER_PASS"

'' end of c-pointer-null-order.bas
