'' Project: FreeBASIC C backend regression tests
'' File: c-code-pointer-load.bas
'' Purpose: Carry typed callable addresses through an opaque library API.
'' Responsibilities: Check nulls, typed calls, lazy selection and both conversion directions.
'' This file intentionally does NOT perform invalid calls or mutate code addresses.
'' The companion C observer is required when linking this fixture.
#lang "fb"

type CodeCallback as function cdecl (byval value as long) as long
type RefCallback as function cdecl (byref value as long) as long
type StdCallback as function stdcall (byval value as long) as long

declare function code_address cdecl alias "fb_test_code_address" (byval which as long) as any ptr
declare function lookup_count cdecl alias "fb_test_lookup_count" (byval which as long) as long
declare function accept_address cdecl alias "fb_test_accept_address" (byval address as any ptr, byval value as long) as long

declare function accept_ref_address cdecl alias "fb_test_accept_ref_address" (byval address as any ptr, byref value as long) as long
declare function accept_std_address cdecl alias "fb_test_accept_std_address" (byval address as any ptr, byval value as long) as long

declare function call_count cdecl alias "fb_test_call_count" () as long

private sub check_local_signature()
	type LocalPayload
		value as long
	end type
	type LocalCallback as function cdecl (byval payload as LocalPayload ptr) as long
	dim as LocalCallback local_null = cptr(LocalCallback, code_address(0))
	if local_null <> 0 then end 20
	dim as any ptr local_opaque = cptr(any ptr, local_null)
	if local_opaque <> 0 then end 21
end sub

dim shared as long basic_calls, callback_evaluations

private function basic_add_ten cdecl (byval value as long) as long
	basic_calls += 1
	return value + 10
end function

private function basic_add_hundred cdecl (byval value as long) as long
	basic_calls += 1
	return value + 100
end function

private function basic_add_ref cdecl (byref value as long) as long
	basic_calls += 1
	value += 7
	return value
end function

private function basic_stdcall stdcall (byval value as long) as long
	basic_calls += 1
	return value + 20
end function

private function make_basic_callback(byval which as long) as CodeCallback
	callback_evaluations += 1
	if which = 1 then return @basic_add_ten
	return @basic_add_hundred
end function

'' Each erased pointer is restored to its exact signature by the C observer.
'' The getter counters distinguish lazy evaluation from a mere correct result.
'' All observers run serially in this one test process.
''
'' The earlier imported-pointer checks also exercise the reverse conversion.

dim as any ptr opaque = code_address(1)
dim as CodeCallback callback = cptr(CodeCallback, opaque)
if callback(41) <> 42 then end 1
if call_count() <> 1 then end 2

dim as CodeCallback null_callback = cptr(CodeCallback, code_address(0))
if null_callback <> 0 then end 3

dim as long gate = 0
callback = iif(gate, cptr(CodeCallback, code_address(1)), cptr(CodeCallback, code_address(2)))
if callback(1) <> 101 then end 4
if lookup_count(1) <> 1 or lookup_count(2) <> 1 then end 5

gate = 1
callback = iif(gate, cptr(CodeCallback, code_address(1)), cptr(CodeCallback, code_address(2)))
if callback(7) <> 8 then end 6
if lookup_count(1) <> 2 or lookup_count(2) <> 1 then end 7

check_local_signature()
if lookup_count(0) <> 2 then end 8
if call_count() <> 3 then end 9
print "CODE_POINTER_LOAD_PASS calls=3 single_evaluation=1 lazy=1 null=1"

opaque = cptr(any ptr, callback)
if accept_address(opaque, 7) <> 8 then end 30
if call_count() <> 4 then end 31

opaque = @basic_add_ten
if accept_address(opaque, 7) <> 17 then end 32
if basic_calls <> 1 then end 33

dim as CodeCallback typed_null = 0
opaque = cptr(any ptr, typed_null)
if opaque <> 0 then end 34
if accept_address(opaque, 7) <> -1 then end 35

gate = 0
opaque = iif(gate, cptr(any ptr, make_basic_callback(1)), cptr(any ptr, make_basic_callback(2)))
if callback_evaluations <> 1 then end 36
if accept_address(opaque, 3) <> 103 then end 37

gate = 1
opaque = iif(gate, cptr(any ptr, make_basic_callback(1)), cptr(any ptr, make_basic_callback(2)))
if callback_evaluations <> 2 then end 38
if accept_address(opaque, 7) <> 17 then end 39

callback = cptr(CodeCallback, cptr(any ptr, make_basic_callback(1)))
if callback_evaluations <> 3 then end 40
if callback(8) <> 18 then end 41
if basic_calls <> 4 then end 42
print "CODE_POINTER_ERASURE_PASS calls=4 single_evaluation=1 lazy=1 null=1"

dim as long payload = 1
opaque = @basic_add_ref
if accept_ref_address(opaque, payload) <> 8 then end 43
if payload <> 8 then end 44
dim as RefCallback ref_callback = cptr(RefCallback, opaque)
if ref_callback(payload) <> 15 then end 45
if payload <> 15 then end 46

opaque = @basic_stdcall
if accept_std_address(opaque, 5) <> 25 then end 47
dim as StdCallback std_callback = cptr(StdCallback, opaque)
if std_callback(6) <> 26 then end 48
if basic_calls <> 8 then end 49
if call_count() <> 4 then end 50
print "CODE_POINTER_SIGNATURES_PASS byref=1 stdcall=1 calls=8"

'' end of c-code-pointer-load.bas
