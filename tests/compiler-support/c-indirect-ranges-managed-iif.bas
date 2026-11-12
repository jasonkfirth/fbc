/'
	Project: FreeBASIC C backend regression controls
	File: c-indirect-ranges-managed-iif.bas
	Purpose: Preserve managed IIF branches outside scalar range rewriting.
	Responsibilities: Check condition evaluation and selected string-producing arms.
	This file contains no graphics, external libraries or filesystem operations.
'/
#lang "fb"

type ManagedConditionSample
	value as ubyte
end type

dim shared as ManagedConditionSample sample
dim shared as integer address_calls, arm_calls

private function NextCondition() as ManagedConditionSample ptr
	address_calls += 1
	return @sample
end function

private function TextArm(byval value as integer) as string
	arm_calls += 1
	return str(value)
end function

for value as integer = 0 to 255
	sample.value = cubyte(value)
	address_calls = 0
	arm_calls = 0
	dim as string result = iif(NextCondition()->value < 0, TextArm(11), TextArm(22))
	if result <> "22" or address_calls <> 1 or arm_calls <> 1 then end 1
	result = iif(NextCondition()->value <= 255, TextArm(33), TextArm(44))
	if result <> "33" or address_calls <> 2 or arm_calls <> 2 then end 2
	result = iif(NextCondition()->value < 128, TextArm(55), TextArm(66))
	if result <> iif(value < 128, "55", "66") or address_calls <> 3 or arm_calls <> 3 then end 3
next
print "C_INDIRECT_RANGES_MANAGED_IIF_PASS"

'' end of c-indirect-ranges-managed-iif.bas
