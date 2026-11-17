/'
    Project: FreeBASIC C backend regression
    File: c-pointer-equality-views.bas
    Purpose: Compare object addresses across declared pointer views.
    Responsibilities: Check nested qualifiers, records, nulls and evaluation counts.
    Stored values and surrounding bytes remain owned by their original objects.
'/
#lang "fb"

type FirstRecord
	value as long
end type

type SecondRecord
	value as double
end type

union PointerSlots
	readOnlyValue as const long ptr
	writeValue as long ptr
end union

type CallbackA as function cdecl(byval as long) as long
type CallbackB as function cdecl(byval as double) as long

dim shared as long leftVisits, rightVisits

function compareNested cdecl alias "probe_compare_nested" _
	(byval lhs as const long ptr ptr, byval rhs as long ptr ptr) as long
	return cint(lhs = rhs)
end function

function unequalNested cdecl alias "probe_unequal_nested" _
	(byval lhs as const long ptr ptr, byval rhs as long ptr ptr) as long
	return cint(lhs <> rhs)
end function

function compareTriple cdecl alias "probe_compare_triple" _
	(byval lhs as const long ptr ptr ptr, byval rhs as long ptr ptr ptr) as long
	return cint(lhs = rhs)
end function

function compareConstSlot cdecl alias "probe_compare_const_slot" _
	(byval lhs as long const ptr ptr, byval rhs as long ptr ptr) as long
	return cint(lhs = rhs)
end function

function compareRecords cdecl alias "probe_compare_records" _
	(byval lhs as FirstRecord ptr, byval rhs as SecondRecord ptr) as long
	return cint(lhs = rhs)
end function

function compareElements cdecl alias "probe_compare_elements" _
	(byval lhs as const long ptr, byval rhs as long ptr) as long
	return cint(lhs = rhs)
end function

function compareErased cdecl alias "probe_compare_erased" _
	(byval lhs as any ptr, byval rhs as const long ptr ptr) as long
	return cint(lhs = rhs)
end function

private function constOperand(byval value as any ptr) as const long ptr ptr
	leftVisits += 1
	return cptr(const long ptr ptr, value)
end function

private function mutableOperand(byval value as any ptr) as long ptr ptr
	rightVisits += 1
	return cptr(long ptr ptr, value)
end function

function compareEvaluated cdecl alias "probe_compare_evaluated" _
	(byval lhs as any ptr, byval rhs as any ptr, byval unequal as long) as long
	leftVisits = 0
	rightVisits = 0
	dim result as long
	if unequal then
		result = cint(constOperand(lhs) <> mutableOperand(rhs))
	else
		result = cint(constOperand(lhs) = mutableOperand(rhs))
	end if
	if leftVisits <> 1 or rightVisits <> 1 then return 17
	return result
end function

function compareProcedures cdecl alias "probe_compare_procedures" _
	(byval lhs as uinteger, byval rhs as uinteger) as long
	return cint(cast(CallbackA, lhs) = cast(CallbackB, rhs))
end function

private function readOnlySlot(byval record as PointerSlots ptr) as const long ptr ptr
	return @record->readOnlyValue
end function

function compareMembers cdecl alias "probe_compare_members" _
	(byval lhs as PointerSlots ptr, byval rhs as PointerSlots ptr) as long
	return cint(readOnlySlot(lhs) = @rhs->writeValue)
end function

#ifdef POINTER_COMPARE_CONST_DROP
function compareExplicitDrop cdecl alias "probe_compare_explicit_drop" _
	(byval lhs as const long ptr ptr, byval rhs as long ptr ptr) as long
	return cint(cptr(long ptr ptr, lhs) = rhs)
end function
#endif

#ifdef POINTER_COMPARE_BAD_SCALAR
function compareBadScalar(byval lhs as long ptr, byval rhs as double) as long
	return cint(lhs = rhs)
end function
#endif

#ifndef POINTER_COMPARE_LIBRARY
dim values(0 to 7) as uinteger = {0, 1, 255, &h7fffffffUL, &h80000000UL, &hffffffffUL, 0, 0}
values(6) = cuint(1) shl (sizeof(any ptr) * 8 - 1)
values(7) = not cuint(0)
for first as long = 0 to 7
	for second as long = 0 to 7
		dim lhs as any ptr = cptr(any ptr, values(first))
		dim rhs as any ptr = cptr(any ptr, values(second))
		dim equalValue as long = cint(values(first) = values(second))
		if compareNested(lhs, rhs) <> equalValue then end 1
		if unequalNested(lhs, rhs) <> not equalValue then end 2
		if compareTriple(lhs, rhs) <> equalValue then end 3
		if compareRecords(lhs, rhs) <> equalValue then end 4
		if compareEvaluated(lhs, rhs, 0) <> equalValue then end 5
		if compareEvaluated(lhs, rhs, 1) <> not equalValue then end 6
		if compareProcedures(values(first), values(second)) <> equalValue then end 7
	next
next
dim slots(0 to 1) as PointerSlots
if compareMembers(@slots(0), @slots(0)) <> -1 then end 8
if compareMembers(@slots(0), @slots(1)) <> 0 then end 9
print "POINTER_EQUALITY_VIEWS_PASS"
#endif

/' end of c-pointer-equality-views.bas '/
