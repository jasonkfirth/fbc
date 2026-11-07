'' Project: FreeBASIC C backend tests
'' File: c-pointer-values.bas
'' Purpose: Compare native pointer-value formation and valid memory accesses.
'' Responsibilities: Scaling, modular offsets, qualifiers and operand evaluation.
'' This file intentionally does NOT dereference out-of-storage addresses.

type ThreeBytes
	first as ubyte
	second as ubyte
	third as ubyte
end type

type EightBytes
	value as longint
end type

dim shared as integer origin_calls, displacement_calls, checked_cases

sub CheckAddress(byval actual as const any ptr, byval expected as uinteger)
	if cuint(actual) <> expected then end 2
	checked_cases += 1
end sub

function StepByte(byval origin as ubyte ptr, byval displacement as integer) as ubyte ptr
	return origin + displacement
end function

function StepThree(byval origin as ThreeBytes ptr, byval displacement as integer) as ThreeBytes ptr
	return origin + displacement
end function

function StepEight(byval origin as EightBytes ptr, byval displacement as integer) as EightBytes ptr
	return origin + displacement
end function

function StepSlot(byval origin as any ptr ptr, byval displacement as integer) as any ptr ptr
	return origin + displacement
end function

function StepConst(byval origin as const ubyte ptr, byval displacement as integer) as const ubyte ptr
	return origin - displacement
end function

sub StepInPlace(byref origin as ubyte ptr, byval displacement as integer)
	origin += displacement
end sub

function OriginValue() as ubyte ptr
	origin_calls += 1
	return 0
end function

function DisplacementValue() as integer
	displacement_calls += 1
	return -1
end function

'' Address values have a native unsigned representation. These reference
'' calculations are independent integer expressions, also checked by GAS64.
dim offsets(0 to 6) as integer = {0, 1, -1, 17, -17, _
	cint(cuint(1) shl (sizeof(integer) * 8 - 1)), _
	cint(not (cuint(1) shl (sizeof(integer) * 8 - 1)))}
dim origins(0 to 3) as uinteger = {0, 1, not cuint(0), not cuint(0) - 16}
for base_index as integer = 0 to 3
	for offset_index as integer = 0 to 6
		var origin = origins(base_index)
		var displacement = offsets(offset_index)
		CheckAddress(StepByte(cptr(ubyte ptr, origin), displacement), origin + cuint(displacement))
		CheckAddress(StepThree(cptr(ThreeBytes ptr, origin), displacement), origin + cuint(displacement) * 3)
		CheckAddress(StepEight(cptr(EightBytes ptr, origin), displacement), origin + cuint(displacement) * 8)
		CheckAddress(StepSlot(cptr(any ptr ptr, origin), displacement), origin + cuint(displacement) * sizeof(any ptr))
		CheckAddress(StepConst(cptr(const ubyte ptr, origin), displacement), origin - cuint(displacement))
		var updated = cptr(ubyte ptr, origin)
		StepInPlace(updated, displacement)
		CheckAddress(updated, origin + cuint(displacement))
	next
next

CheckAddress(OriginValue() + DisplacementValue(), not cuint(0))
CheckAddress(DisplacementValue() + OriginValue(), not cuint(0))
if origin_calls <> 2 or displacement_calls <> 2 then end 3

'' Only these accesses read or write memory, and every target stays inside
'' its declared array. One-past and other address-only cases above are not used.
dim storage(0 to 127) as ubyte
for index as integer = 0 to 127
	*StepByte(@storage(0), index) = cubyte(index)
	if storage(index) <> cubyte(index) then end 4
	checked_cases += 1
next
dim triples(0 to 15) as ThreeBytes
for index as integer = 0 to 15
	var item = StepThree(@triples(0), index)
	item->first = cubyte(index)
	item->second = cubyte(index + 1)
	item->third = cubyte(index + 2)
	if triples(index).first <> cubyte(index) then end 5
	if triples(index).second <> cubyte(index + 1) then end 6
	if triples(index).third <> cubyte(index + 2) then end 7
	checked_cases += 1
next
dim records(0 to 15) as EightBytes
for index as integer = 0 to 15
	StepEight(@records(0), index)->value = clngint(index) * 101
	if records(index).value <> clngint(index) * 101 then end 8
	checked_cases += 1
next

if checked_cases <> 330 then end 9
print "POINTER_VALUES_PASS"; checked_cases

'' end of c-pointer-values.bas
