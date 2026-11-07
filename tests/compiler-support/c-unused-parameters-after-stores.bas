'' Project: FreeBASIC C backend tests
'' File: c-unused-parameters-after-stores.bas
'' Purpose: Keep required argument evaluation after private stores disappear.
'' Responsibilities: Cover value/reference inputs, static members and callback ABI.
'' This file intentionally does NOT contain graphics or thread scheduling.

#lang "fb"

'' This fixture owns the counter; it observes caller-side argument evaluation.
dim shared as integer argument_calls

private function nextValue() as integer
	argument_calls += 1
	return 41
end function

'' An addressable read-only ABI wrapper is emitted after ordinary bodies.
private function readOnlyValue(byref value as integer) as integer
	return value
end function

extern "C"
private function shortNames(byval i as integer, byval p as integer, byval y as integer) as integer
	return i + argument_calls
end function
end extern

private sub discardedValue(byval value as integer)
	dim as integer unobserved
	unobserved = value
	unobserved = unobserved * 2
end sub

'' BYVAL writes affect only the callee's binding. Removing that binding must
'' still evaluate the incoming argument and each effectful assigned value.
private sub overwrittenValue(byval value as integer)
	value = nextValue()
	value = 0
end sub

private sub overwrittenPointer(byval value as integer ptr)
	value = 0
end sub

private sub changedReference(byref value as integer)
	value = 43
end sub

private sub assignThrough(byval target as integer ptr)
	*target = 47
end sub

private function escapedValue(byval value as integer) as integer
	value = 43
	assignThrough(@value)
	return value
end function

private sub discardedReference(byref value as const integer)
	dim as integer unobserved
	unobserved = value * 3
end sub

type SharedFields
	unused as integer
	static total as integer
end type

dim SharedFields.total as integer

private sub onlyShared(byval context as any ptr)
	with *cptr(SharedFields ptr, context)
		.total += 1
	end with
end sub

dim as integer input_value = nextValue()
dim read_callback as function(byref as integer) as integer = @readOnlyValue
if read_callback(input_value) <> 41 then end 2
discardedValue(nextValue())
discardedReference(input_value)

dim as SharedFields context
dim callback as sub(byval as any ptr) = @onlyShared
callback(@context)

if shortNames(40, 1, 2) <> 42 then end 3

overwrittenValue(nextValue())
overwrittenPointer(@input_value)
if input_value <> 41 then end 4
changedReference(input_value)
if input_value <> 43 then end 5
if escapedValue(1) <> 47 then end 6

if argument_calls <> 4 orelse SharedFields.total <> 1 then end 1
print "C_UNUSED_PARAMETERS_AFTER_STORES_PASS"

'' end of c-unused-parameters-after-stores.bas
