/'
	Project: FreeBASIC C backend regression controls
	File: c-unused-store-bindings.bas
	Purpose: Remove unread storage while retaining observable evaluations.
	Responsibilities: Dead loop counters, pointer/reference bindings, incoming
	branches, escaped addresses, initialization calls and object lifetimes.
	This file intentionally does NOT contain: Platform APIs or filesystem work.
'/

#macro VERIFY(condition)
	if (condition) = 0 then
		print "C_UNUSED_STORE_BINDINGS_FAILED", __LINE__
		end 1
	end if
#endmacro

'' The counters make every retained call and object lifetime observable.
dim shared as integer effect_calls, reference_calls, constructors, destructors
dim shared as integer shared_number = 37
dim shared as string shared_text

type OwnedValue
	marker as integer
	declare constructor()
	declare destructor()
end type

constructor OwnedValue()
	constructors += 1
	marker = 19
end constructor

destructor OwnedValue()
	VERIFY(marker = 19)
	destructors += 1
end destructor

private function EffectValue() as integer
	effect_calls += 1
	return 71
end function

private function NumberReference() byref as integer
	reference_calls += 1
	return shared_number
end function

private function TextReference() byref as string
	reference_calls += 1
	return shared_text
end function

private sub UnreadBindings()
	dim as OwnedValue owned
	dim as OwnedValue ptr unused_address = @owned
	dim byref unused_object as OwnedValue = owned
	dim byref unused_text as string = TextReference()
	dim as integer ptr unused_pointer = @shared_number
	dim byref unused_number as integer = NumberReference()
	dim as integer unread_value = EffectValue()
	goto after_unreachable_read
	unread_value += 1
	VERIFY(0)
after_unreachable_read:
	for empty_index as long = 1 to 0
		VERIFY(0)
	next
	for reverse_index as long = 0 to 1 step -1
		VERIFY(0)
	next
end sub

private sub LiveBindings()
	dim as integer original_value = 7, replacement_value = 9
	dim as integer ptr original_pointer = @original_value
	dim byref value_alias as integer = original_value
	dim byref pointer_alias as integer ptr = original_pointer
	value_alias = 11
	VERIFY(original_value = 11)
	pointer_alias = @replacement_value
	VERIFY(*original_pointer = 9)
	dim as integer reached = 13
	goto incoming_target
	reached = 0
incoming_target:
	VERIFY(reached = 13)
	dim as integer iterations
	for live_index as integer = -3 to 3
		iterations += 1
	next
	VERIFY(iterations = 7)
end sub

shared_text = "retained"
UnreadBindings()
VERIFY(effect_calls = 1)
VERIFY(reference_calls = 2)
VERIFY(constructors = 1)
VERIFY(destructors = 1)
VERIFY(shared_number = 37)
VERIFY(shared_text = "retained")
LiveBindings()
print "C_UNUSED_STORE_BINDINGS_PASS"

'' end of c-unused-store-bindings.bas
