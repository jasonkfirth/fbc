'' Project: FreeBASIC ordered pointer comparison qualification
'' File: c-pointer-order.bas
'' Purpose: Compare native pointer ordering with unsigned address ordering.
'' Responsibilities: Check full-width values, function addresses and evaluation counts.
'' This file intentionally does NOT dereference constructed address values.
#lang "fb"

dim shared visits(0 to 1) as integer

function operand(byval slot as integer, byval value as any ptr) as any ptr

	visits(slot) += 1
	return value

end function

sub check_pair(byval a as any ptr, byval b as any ptr)

	dim as integer gt = cuint(a) > cuint(b), lt = cuint(a) < cuint(b)
	dim as integer ge = cuint(a) >= cuint(b), le = cuint(a) <= cuint(b)
	if (a > b) <> gt then end 11
	if (a < b) <> lt then end 12
	if (a >= b) <> ge then end 13
	if (a <= b) <> le then end 14
	if (a = b) <> (cuint(a) = cuint(b)) then end 15
	if (a <> b) <> (cuint(a) <> cuint(b)) then end 16
	if (not (a > b)) <> (not gt) then end 17
	dim truth as boolean = a < b
	if truth <> cbool(lt) then end 18
	visits(0) = 0
	visits(1) = 0
	dim result as integer = operand(0, a) >= operand(1, b)
	if result <> ge orelse visits(0) <> 1 orelse visits(1) <> 1 then end 19
	visits(0) = 0
	visits(1) = 0
	if operand(0, a) <= operand(1, b) then
		if le = 0 then end 20
	else
		if le <> 0 then end 21
	end if
	if visits(0) <> 1 orelse visits(1) <> 1 then end 22

end sub

sub callback_a()
end sub

sub callback_b()
end sub

dim addresses(0 to 9) as uinteger
addresses(0) = 0
addresses(1) = 1
addresses(2) = 255
addresses(3) = &h7FFFFFFFUL
addresses(4) = &h80000000UL
addresses(5) = &hFFFFFFFFUL
#ifdef __FB_64BIT__
	addresses(6) = &h100000000ULL
	addresses(7) = &h7FFFFFFFFFFFFFFFULL
	addresses(8) = &h8000000000000000ULL
#else
	addresses(6) = &h10000UL
	addresses(7) = &h7FFFFFFEUL
	addresses(8) = &h80000001UL
#endif
addresses(9) = not cuint(0)
for i as integer = 0 to 9
	for j as integer = 0 to 9
		check_pair(cptr(any ptr, addresses(i)), cptr(any ptr, addresses(j)))
	next
next

'' Actual unrelated objects and procedure addresses use the same ordering.
dim left_object as long, right_object as long
check_pair(@left_object, @right_object)
check_pair(@right_object, @left_object)
dim as sub() left_callback = @callback_a, right_callback = @callback_b
if (left_callback > right_callback) <> (cuint(left_callback) > cuint(right_callback)) then end 23
if (left_callback < right_callback) <> (cuint(left_callback) < cuint(right_callback)) then end 24
if (left_callback >= right_callback) <> (cuint(left_callback) >= cuint(right_callback)) then end 25
if (left_callback <= right_callback) <> (cuint(left_callback) <= cuint(right_callback)) then end 26
if left_callback > 0 then
	if cuint(left_callback) = 0 then end 27
else
	end 28
end if
print "POINTER_ORDER_VALUES_PASS"

'' end of c-pointer-order.bas
