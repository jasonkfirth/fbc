'' Project: FreeBASIC C backend regressions
'' File: c-debug-static-locals.bas
'' Purpose: Describe unread static debug storage without evaluating its value.
'' Responsibilities: Preserve assignments, random draws and static ownership.
'' This file intentionally does NOT read the two unused source statics.
#lang "fb"

dim shared effectCount as long
dim shared initializationCount as long
dim shared destructionCount as long

private function nextValue() as long
	effectCount += 1
	return clng(rnd() * 1000)
end function

type StaticOwner
	markerValue as byte
	declare constructor()
	declare destructor()
end type

constructor StaticOwner()
	markerValue = 17
	initializationCount += 1
	if nextValue() < 0 then end 90
end constructor

destructor StaticOwner()
	destructionCount += 1
	if initializationCount = 1 andalso destructionCount = 1 andalso markerValue = 17 then
		print "STATIC_LOCAL_CLEANUP_PASS"
	else
		print "STATIC_LOCAL_CLEANUP_FAIL"
	end if
end destructor

private sub updateExplicit()
	static owner as StaticOwner
	static assignedValue as long
	assignedValue = nextValue()
end sub

private sub updateAll() static
	dim assignedValue as long
	assignedValue = nextValue()
end sub

'' Both runs use the same fixed seed and BASIC-compatible generator.
randomize 123, 1
for iteration as long = 1 to 3
	updateExplicit()
	updateAll()
next
if initializationCount <> 1 orelse effectCount <> 7 then end 1
dim actualValue as double = rnd()

'' Reconstruct the reference stream independently. The owned static draws
'' once; the two assignments draw on every call, despite never being read.
randomize 123, 1
dim expectedValue as double
for iteration as long = 1 to 8
	expectedValue = rnd()
next
if actualValue <> expectedValue then end 2
print "STATIC_LOCAL_EFFECTS_PASS"

'' end of c-debug-static-locals.bas
