/'
	Project: FreeBASIC C backend regression tests
	File: c-unused-call-results.bas
	Purpose: Keep calls and writes when their integer status has no consumer.
	Responsibilities: Check UTF-8 line input, statement calls, used results and
	argument evaluation, including a procedure with no scalar local binding.
	This file intentionally does NOT contain: Diagnostic suppression or changes
	to the runtime error policy.
'/
#lang "fb"
#include once "crt/unistd.bi"

dim shared as long calls, arguments

private function Argument() as long
	arguments += 1
	return arguments
end function

private function CountCall(byval value as long) as long
	calls += 1
	return value + 10
end function

private sub ReadWithoutScalarBindings(byref text as ustring)
	#ifdef CHECK_OPAQUE
		asm
			nop
		end asm
	#endif
	line input #1, text
end sub

private sub ReadWithScalarBinding(byref text as ustring)
	dim as integer handle = 1
	#ifdef CHECK_OPAQUE
		asm
			nop
		end asm
	#endif
	line input #handle, text
end sub

dim as string filename = "c-unused-call-results-" + str(getpid()) + ".tmp"
dim as ustring expected = uchr(65, &hE9, &h1F600)
dim as ustring first_line, second_line
dim as long failure
if( open(filename for output encoding "utf-8" as #1) <> 0 ) then end 1
print #1, expected
print #1, expected + "two"
close #1
if( open(filename for input encoding "utf-8" as #1) <> 0 ) then end 2
ReadWithoutScalarBindings(first_line)
ReadWithScalarBinding(second_line)
close #1
if( first_line <> expected or second_line <> expected + "two" ) then failure = 3
if( kill(filename) <> 0 ) then failure = 4
CountCall(Argument())
if( CountCall(Argument()) <> 12 ) then failure = 5
if( calls <> 2 or arguments <> 2 ) then failure = 6
if( failure <> 0 ) then end failure
print "C_UNUSED_CALL_RESULTS_PASS"

/' end of c-unused-call-results.bas '/
