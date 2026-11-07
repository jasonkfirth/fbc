/'
 Project: FreeBASIC C backend controls
 File: c-debug-compiler-bindings.bas
 Purpose: Exercise compiler-generated scoped storage and private array metadata.
 Responsibilities: Observe loop bounds, WITH/SELECT evaluation and static arrays.
 This file intentionally does NOT access graphics or uninitialized values.
'/
#lang "fb"

dim shared as integer boundCalls, withCalls, textCalls

type RecordValue
	value as integer
end type
dim shared recordData as RecordValue

private function upperBoundValue() as integer
	boundCalls += 1
	return 3
end function

private function earlyLoop(byval count as integer) as integer
	return 17
	for index as integer = 1 to count + upperBoundValue()
		print index
	next
end function

private function recordAddress() as RecordValue ptr
	withCalls += 1
	return @recordData
end function

private function textValue() as string
	textCalls += 1
	return "alpha"
end function

private sub checkDeadStrings()
	exit sub
	print textValue() + textValue() + textValue()
end sub

private function directStatic(byval index as integer) as long
	static debugStaticData(2 to 4) as long => {11, 22, 33}
	return debugStaticData(index)
end function

private function descriptorSum(values() as long) as long
	dim result as long
	for index as integer = lbound(values) to ubound(values)
		result += values(index)
	next
	return result
end function

private function passedStatic() as long
	static passedStaticData(-1 to 1) as long => {3, 5, 7}
	return descriptorSum(passedStaticData())
end function

private sub liveScopedValues()
	dim total as integer
	for index as integer = 1 to upperBoundValue()
		total += index
	next
	if total <> 6 or boundCalls <> 1 then end 1
	with *recordAddress()
		.value = 31
	end with
	if recordData.value <> 31 or withCalls <> 1 then end 2
	select case wstr(textValue())
	case wstr("alpha")
		total += 1
	case else
		end 3
	end select
	if total <> 7 or textCalls <> 1 then end 4
end sub

if earlyLoop(7) <> 17 or boundCalls <> 0 then end 5
liveScopedValues()
checkDeadStrings()
if directStatic(2) + directStatic(4) <> 44 then end 6
if passedStatic() <> 15 then end 7
print "C_DEBUG_COMPILER_BINDINGS_PASS"

'' end of c-debug-compiler-bindings.bas
