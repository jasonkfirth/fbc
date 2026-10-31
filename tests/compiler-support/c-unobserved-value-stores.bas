#lang "fb"
/'
	Project: FreeBASIC C backend regression tests
	File: c-unobserved-value-stores.bas
	Purpose: Verify removal of unread private record, array and reference stores.
	Responsibilities: Check call order, live and escaped storage, BYREF carriers,
	and the constructor/destructor ownership of referred objects.
	This file intentionally does NOT contain: Diagnostic suppression or invalid
	addresses used to simulate a live object.
'/

#macro VERIFY(condition)
	if (condition) = 0 then end 1
#endmacro

#macro OPAQUE_BOUNDARY()
	#ifdef CHECK_OPAQUE
		asm
			nop
		end asm
	#endif
#endmacro

type PlainRecord
	first as long
	last as long
end type

dim shared as ulongint events
dim shared as long index_calls, constructed, destroyed

private function NextValue(byval value as long) as long
	events = events * 10 + value
	return value
end function

private function NextIndex() as long
	index_calls += 1
	return 0
end function

private sub DropRecords()
	OPAQUE_BOUNDARY()
	dim dead as PlainRecord = (NextValue(1), NextValue(2))
	dim plain as PlainRecord = (3, 4)
	plain.last = 5
end sub

private sub DropArrays()
	OPAQUE_BOUNDARY()
	dim dead_values(0 to 2) as long
	dead_values(NextIndex()) = NextValue(3)
	dead_values(1) = NextValue(4)
	dim dead_pointers(0 to 1) as any ptr
	dead_pointers(0) = @events
end sub

private sub ChangeRecord(byref value as PlainRecord)
	value.first = 77
	value.last = 88
end sub

private sub ChangeArray(values() as long)
	values(0) = 91
	values(1) = 92
end sub

private sub CheckDestinationIndexes()
	OPAQUE_BOUNDARY()
	dim values(0 to 3) as long
	dim position as long = 0
	values(position) = 31
	position += 1
	values(position * 2) = 32
	VERIFY(values(0) = 31 and values(2) = 32)
end sub

private sub CheckObservedStorage()
	OPAQUE_BOUNDARY()
	dim record as PlainRecord = (6, 7)
	VERIFY(record.first = 6 and record.last = 7)
	ChangeRecord(record)
	VERIFY(record.first = 77 and record.last = 88)
	dim values(0 to 1) as long = {8, 9}
	ChangeArray(values())
	VERIFY(values(0) = 91 and values(1) = 92)
end sub

type Guard
	label as string
	declare constructor()
	declare destructor()
end type

constructor Guard()
	constructed += 1
	label = "alive"
end constructor

destructor Guard()
	VERIFY(label = "alive")
	destroyed += 1
end destructor

type OwnedState
	owner as Guard
	values(any) as long
	cookie as long
end type

private function DropReference(byref state as OwnedState) as long
	OPAQUE_BOUNDARY()
	'' STATE's physical BYREF pointer does not own the object. Removing this
	'' unread alias must keep the index call and STATE's ordinary lifetime.
	var byref unread = state.values(NextIndex())
	return state.cookie
end function

private function ReadReference(byref state as OwnedState) as long
	OPAQUE_BOUNDARY()
	var byref observed = state.values(0)
	observed += 5
	return observed
end function

private function ReferenceAddress(byref state as OwnedState) as long ptr
	OPAQUE_BOUNDARY()
	var byref escaped = state.values(1)
	return @escaped
end function

private function ArrayUpper(values() as long) as long
	return ubound(values)
end function

for repeat as long = 1 to 3
	events = 0
	index_calls = 0
	DropRecords()
	VERIFY(events = 12)
	DropArrays()
	VERIFY(events = 1234)
	VERIFY(index_calls = 1)
	CheckObservedStorage()
	CheckDestinationIndexes()
	scope
		dim state as OwnedState
		state.cookie = 42
		redim state.values(0 to 1)
		state.values(0) = 10
		state.values(1) = 20
		VERIFY(ArrayUpper(state.values()) = 1)
		VERIFY(DropReference(state) = 42)
		VERIFY(index_calls = 2)
		VERIFY(ReadReference(state) = 15)
		VERIFY(state.values(0) = 15)
		dim address as long ptr = ReferenceAddress(state)
		VERIFY(address = @state.values(1))
		*address = 29
		VERIFY(state.values(1) = 29)
		VERIFY(constructed = repeat and destroyed = repeat - 1)
	end scope
	VERIFY(constructed = repeat and destroyed = repeat)
next

print "C_UNOBSERVED_VALUE_STORES_PASS"

/' end of c-unobserved-value-stores.bas '/
