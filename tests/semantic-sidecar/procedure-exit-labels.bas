'' Project: FreeBASIC semantic sidecar tests
'' File: procedure-exit-labels.bas
'' Purpose: Exercise routine exit identities independently of exit spelling.
'' Responsibilities: Loop exits, scope cleanup, returns and macro transfers.
'' This file intentionally does NOT execute file operations or compiler output.
#lang "fb"

#define LeaveRoutine() Exit Sub

sub LeaveThroughMacro()
	LeaveRoutine()
end sub

sub LeaveLoops(byval choose as integer)
	for counter as integer = 0 to 3
		if choose then exit for
	next
	do
		if choose then exit do
		exit sub
	loop
end sub

function ReturnNumber(byval choose as integer) as integer
	if choose then return 1
	ReturnNumber = 2
end function

function ReturnText(byval choose as integer) as string
	dim contents as string = "scope cleanup"
	if choose then return contents
	return "other"
end function

type Lifetime
	numberValue as integer
	declare constructor()
	declare destructor()
	declare property Number() as integer
end type

constructor Lifetime()
	numberValue = 3
	exit constructor
end constructor

destructor Lifetime()
	exit destructor
end destructor

property Lifetime.Number() as integer
	return numberValue
end property

sub DestroyOnExit()
	dim instance as Lifetime
	exit sub
end sub

LeaveThroughMacro()
'' end of procedure-exit-labels.bas
