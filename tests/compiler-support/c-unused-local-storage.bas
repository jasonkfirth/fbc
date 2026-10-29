'' Project: FreeBASIC C backend regression tests
'' File: c-unused-local-storage.bas
'' Purpose: Keep observable storage while omitting locals in discarded C statements.
'' Responsibilities: Check reachability, hidden results, copyback and object lifetime.
'' This file intentionally does NOT depend on unused-variable suppression.
#lang "fb"

dim shared effects as long
dim shared living as long

type Payload
	words(0 to 3) as longint
end type

type Counted
	marker as long
	declare constructor()
	declare destructor()
end type

constructor Counted()
	marker = 1
	living += 1
end constructor

destructor Counted()
	living -= 1
end destructor

private function make_payload(byval value as longint) as Payload
	dim result as Payload
	result.words(0) = value
	effects += 1
	return result
end function

private function observe() as long
	effects += 100
	return effects
end function

private sub change_text(byref value as string)
	value = "changed"
end sub

private sub check_dead_scope()
	goto continued
	scope
		dim dead_object as Counted
		dim dead_value as long = observe()
		if dead_value = 0 then effects += 1000
	end scope
continued:
	if living <> 0 or effects <> 0 then end 1
end sub

check_dead_scope()
if observe() <> 100 then end 9
effects = 0
scope
	dim active as Counted
	if living <> 1 then end 2
end scope
if living <> 0 then end 3

'' Even an ignored large result needs caller-owned ABI storage.
make_payload(7)
if effects <> 1 then end 4
scope
	dim actual as Payload = make_payload(9)
	if actual.words(0) <> 9 then end 5
end scope
if effects <> 2 then end 6

'' BYREF dynamic-string arguments borrow a temporary for fixed-string copyback.
dim fixed_text as string * 16
change_text(fixed_text)
if rtrim(fixed_text) <> "changed" then end 7

static numbers(0 to 2) as long
numbers(1) = 42
if numbers(1) <> 42 then end 8
print "C_UNUSED_LOCAL_STORAGE_PASS"
'' end of c-unused-local-storage.bas
