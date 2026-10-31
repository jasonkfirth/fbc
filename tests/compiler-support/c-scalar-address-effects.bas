/'
	Project: FreeBASIC C backend regression controls
	File: c-scalar-address-effects.bas
	Purpose: Discard unused address bindings while retaining their evaluations.
	Responsibilities: Constructor ownership, destructor order, indexed lvalues,
	field addresses, live aliases and borrowed string concatenation results.
	This file intentionally does NOT contain: Platform APIs or allocation failures.
'/
#lang "fb"
#include once "../virtual/fbrtti.bi"

#macro VERIFY(condition)
	if (condition) = 0 then
		print "C_SCALAR_ADDRESS_EFFECTS_FAILED", __LINE__
		end 1
	end if
#endmacro

dim shared as string events
dim shared as integer created, destroyed, index_calls, target_calls
dim shared as object target_object

type Element extends object
	payload as string
	tag as integer
	declare constructor(byval value as integer = 0)
	declare destructor()
end type

constructor Element(byval value as integer)
	tag = value
	payload = "live" + str(value)
	created += 1
	events += "C" + str(tag) + ";"
end constructor

destructor Element()
	VERIFY(payload = "live" + str(tag))
	destroyed += 1
	events += "D" + str(tag) + ";"
end destructor

private function TargetAddress() as object ptr
	target_calls += 1
	events += "L;"
	return @target_object
end function

private function IndexValue(byval side as integer) as integer
	index_calls += 1
	events += "I" + str(side) + ";"
	return side
end function

private sub ConstructorResults()
	dim as any ptr original_vptr = getvptr(@target_object)
	events = ""
	target_object = Element(1)
	VERIFY(events = "C1;D1;")
	VERIFY(created = 1 and destroyed = 1)
	events = ""
	*TargetAddress() = Element(2)
	VERIFY(events = "L;C2;D2;")
	VERIFY(target_calls = 1 and created = 2 and destroyed = 2)
	VERIFY(getvptr(@target_object) = original_vptr)
end sub

private sub IndexedObjects()
	dim as object values(0 to 1)
	events = ""
	target_object = values(IndexValue(0))
	values(IndexValue(1)) = target_object
	values(IndexValue(0)) = values(IndexValue(1))
	VERIFY(events = "I0;I1;I0;I1;")
	VERIFY(index_calls = 4)
	events = ""
	scope
		dim as Element owners(0 to 1)
		VERIFY(created = 4 and destroyed = 2)
		target_object = owners(IndexValue(1))
		VERIFY(created = 4 and destroyed = 2)
	end scope
	VERIFY(events = "C0;C0;I1;D0;D0;")
	VERIFY(created = destroyed and index_calls = 5)
end sub

type Container
	padding as integer
	value as object
end type

private sub ExistingStorage()
	dim as Container holder
	'' WITH evaluates its designator even when its body uses no field.
	with holder.value
	end with
	with target_object
	end with
	dim as integer value = 0
	dim as integer ptr live_alias = @value
	*live_alias = 57
	VERIFY(value = 57)
end sub

private function DeadTarget(byval value as integer) as integer
	return value + 1
end function

private function LiveTarget(byval value as integer) as integer
	return value + 2
end function

private sub ProcedureAddresses()
	dim as function(byval as integer) as integer unused_alias = @DeadTarget
	dim as function(byval as integer) as integer live_alias = @LiveTarget
	VERIFY(live_alias(55) = 57)
	VERIFY(target_calls = 1)
end sub

private function StringAddress(byref value as string) as string ptr
	target_calls += 1
	return @value
end function

private sub BorrowedResults()
	dim as string value = "a"
	*StringAddress(value) += "b"
	VERIFY(value = "ab" and target_calls = 2)
	*StringAddress(value) += value
	VERIFY(value = "abab" and target_calls = 3)
end sub

#ifdef CHECK_OPAQUE
	asm
		nop
	end asm
#endif

ConstructorResults()
IndexedObjects()
ExistingStorage()
ProcedureAddresses()
BorrowedResults()
print "C_SCALAR_ADDRESS_EFFECTS_PASS"

/' end of c-scalar-address-effects.bas '/
