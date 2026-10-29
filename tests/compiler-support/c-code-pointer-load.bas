'' Project: FreeBASIC C backend regression tests
'' File: c-code-pointer-load.bas
'' Purpose: Load callable addresses returned through an opaque library API.
'' Responsibilities: Check nulls, typed calls, lazy selection and local signatures.
'' This file intentionally does NOT perform invalid calls or mutate code addresses.
'' The companion C observer is required when linking this fixture.
#lang "fb"

type CodeCallback as function cdecl (byval value as long) as long

declare function code_address cdecl alias "fb_test_code_address" (byval which as long) as any ptr
declare function lookup_count cdecl alias "fb_test_lookup_count" (byval which as long) as long
declare function call_count cdecl alias "fb_test_call_count" () as long

private sub check_local_signature()
	type LocalPayload
		value as long
	end type
	type LocalCallback as function cdecl (byval payload as LocalPayload ptr) as long
	dim as LocalCallback local_null = cptr(LocalCallback, code_address(0))
	if local_null <> 0 then end 20
end sub

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

'' end of c-code-pointer-load.bas
