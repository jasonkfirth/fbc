/'
 Project: FreeBASIC C backend controls
 File: c-debug-implicit-user.bas
 Purpose: Distinguish legacy user declarations from compiler-owned variables.
 Responsibilities: Keep a source watch binding while checking a live value.
 This file intentionally does NOT read the unused watch value.
'/
#lang "fblite"

sub checkImplicit()
	watchImplicit = 23
	userImplicit = 17
	if userImplicit <> 17 then end 1
end sub

checkImplicit()
print "C_DEBUG_IMPLICIT_USER_PASS"

'' end of c-debug-implicit-user.bas
