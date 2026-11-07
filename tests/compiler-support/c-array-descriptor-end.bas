/'
	Project: FreeBASIC C backend regression tests
	File: c-array-descriptor-end.bas
	Purpose: Keep reachable array lifetimes without metadata used only after END.
	Responsibilities: Observe managed elements, a reachable branch target and
	string values before explicit termination.
	This file intentionally does NOT depend on a database or external files.
'/
#lang "fb"

dim shared as integer constructors, destructors
type OwnedText
	text as string
	declare constructor()
	declare destructor()
end type
constructor OwnedText()
	constructors += 1
	text = "owned"
end constructor
destructor OwnedText()
	if text <> "owned" then end 2
	destructors += 1
end destructor

private sub lifetimeValues()
	dim entries(0 to 1) as OwnedText
	goto observed
	end 3
observed:
	if entries(0).text <> "owned" or entries(1).text <> "owned" then end 4
end sub
lifetimeValues()
if constructors <> 2 or destructors <> 2 then end 5

dim fields(0 to 24) as string
fields(0) = "alpha"
fields(1) = "beta"
if fields(0) <> "alpha" or fields(1) <> "beta" then end 1
print "ARRAY_DESCRIPTOR_END_PASS"
end 0

'' end of c-array-descriptor-end.bas
