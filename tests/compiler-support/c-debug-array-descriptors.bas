'' Project: FreeBASIC C backend regression tests
'' File: c-debug-array-descriptors.bas
'' Purpose: Exercise fixed array metadata in debug builds.
'' Responsibilities: Preserve debugger-visible data, descriptor arguments,
''				  initialization effects and opaque assembly.
'' This file intentionally does NOT require a particular optimizer.
#lang "fb"

dim shared effects as long
dim shared constructed as long
dim shared destroyed as long

type Counted
	value as long
	declare constructor()
	declare destructor()
end type

constructor Counted()
	value = 7
	constructed += 1
end constructor

destructor Counted()
	destroyed += 1
end destructor

private function bump() as long
	effects += 1
	return effects
end function

private sub checkDirect()
	dim values(1 to 4) as long = { 11, 22, 33, 44 }
	if values(1) <> 11 or values(4) <> 44 then end 1
	dim initialized(1 to 2) as long = { bump(), bump() }
	dim objects(1 to 2) as Counted
	if constructed <> 2 or objects(1).value <> 7 then end 2
end sub

private function sumDescriptor(values() as long) as long
	dim result as long
	for i as integer = lbound(values) to ubound(values)
		result += values(i)
	next
	return result
end function

private sub checkPassed()
	dim values(-1 to 1) as long = { 3, 5, 7 }
	if sumDescriptor(values()) <> 15 then end 3
end sub

private sub checkOpaque()
	dim values(1 to 2) as long = { 17, 19 }
	asm
		nop
	end asm
	if values(1) <> 17 or values(2) <> 19 then end 4
end sub

private sub checkAfterOpaque()
	dim values(1 to 2) as long = { 23, 29 }
	if values(1) <> 23 or values(2) <> 29 then end 5
end sub

checkDirect()
if effects <> 2 or constructed <> 2 or destroyed <> 2 then end 6
checkPassed()
checkOpaque()
checkAfterOpaque()
print "C_DEBUG_ARRAY_DESCRIPTORS_PASS"

'' end of c-debug-array-descriptors.bas
