'' Project: FreeBASIC C backend regression tests
'' File: c-debug-unused-temporaries.bas
'' Purpose: Check unused internal locals in debug output.
'' Responsibilities: Preserve source locals, live evaluations and opaque
''                  assembly while removing unreachable temporary storage.
'' This file intentionally does NOT open a graphics window.
#lang "fb"

dim shared evaluations as long

private function bump() as long
	evaluations += 1
	return evaluations
end function

private sub checkDead()
	exit sub
	dim screenWidth as integer, screenHeight as integer
	screeninfo screenWidth, screenHeight
end sub

private sub checkOpaque()
	asm
		nop
	end asm
	exit sub
	dim screenWidth as integer, screenHeight as integer
	screeninfo screenWidth, screenHeight
end sub

private sub checkVisible()
	dim watchOnly as long = any
	dim userValue as long = 19
	if userValue <> 19 then end 1
	dim activeValue as long = bump()
	if activeValue <> 1 then end 2
end sub

private sub checkJumpTarget()
	goto assigned
	dim targetValue as long = any
assigned:
	targetValue = 37
	if targetValue <> 37 then end 4
end sub

checkDead()
checkOpaque()
checkVisible()
checkJumpTarget()
if evaluations <> 1 then end 3
print "C_DEBUG_UNUSED_TEMPORARIES_PASS"

'' end of c-debug-unused-temporaries.bas
