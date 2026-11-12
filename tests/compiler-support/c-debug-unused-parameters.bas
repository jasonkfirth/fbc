'' Project: FreeBASIC C backend regression tests
'' File: c-debug-unused-parameters.bas
'' Purpose: Keep debugger-visible formal storage for ignored ABI arguments.
'' Responsibilities: Check caller evaluation, pointer/reference/descriptor
''                  parameters and the conservative opaque-assembly boundary.
'' This file intentionally does NOT access ignored pointer values.

#lang "fb"

type DebugRecord
	value as long
end type

'' This fixture owns the counter and observes caller-side evaluation only.
dim shared as long evaluations

private function nextArgument() as long
	evaluations += 1
	return 41
end function

private sub ignoredDebug(byval value as long, byval pointerValue as long ptr, _
	byref referenceValue as long, arrayValue() as long, _
	byval callbackValue as sub(), byval recordValue as DebugRecord)
	evaluations += 1
end sub

private function observedDebug(byval value as long, byval unusedValue as long) as long
	return value + evaluations
end function

#ifdef DEBUG_OPAQUE_CONTROL
private sub opaqueDebug(byval opaqueValue as long)
	'' General assembly may name formal storage outside typed IR references.
	asm
		nop
	end asm
end sub
opaqueDebug(1)
#endif

dim as long referenceValue = 19
dim as long arrayValue(0 to 1) = {3, 5}
dim as DebugRecord recordValue = (23)
ignoredDebug(nextArgument(), 0, referenceValue, arrayValue(), 0, recordValue)
if evaluations <> 2 then end 1
if observedDebug(40, nextArgument()) <> 43 then end 2
if evaluations <> 3 then end 3
if referenceValue <> 19 then end 4
if arrayValue(0) <> 3 orelse arrayValue(1) <> 5 then end 5
if recordValue.value <> 23 then end 6
print "C_DEBUG_UNUSED_PARAMETERS_PASS"

'' end of c-debug-unused-parameters.bas
