'' Project: FreeBASIC generic callback representation qualification
'' File: c-generic-callback-values.bas
'' Purpose: Preserve callback values across generic erasure and restoration.
'' Responsibilities: Check calls, identity, null values, lazy evaluation and one evaluation.
'' This file intentionally does NOT invoke an incompatible callback signature.
#lang "fb"

type TypedCallback as function cdecl(byval as long, byval as any ptr) as long
type ErasedCallback as sub cdecl()
declare function typed_callback cdecl(byval value as long, byval userdata as any ptr) as long
declare function callback_probe cdecl alias "callback_probe" (byval callback as ErasedCallback, byval value as long, byval userdata as any ptr) as long
declare function callback_echo cdecl alias "callback_echo" (byval callback as ErasedCallback) as ErasedCallback
declare function callback_get cdecl alias "callback_get" () as ErasedCallback
declare function callback_null cdecl alias "callback_null" () as TypedCallback
declare function callback_is_null cdecl alias "callback_is_null" (byval callback as ErasedCallback) as long
declare function callback_gate cdecl alias "callback_gate" () as long

dim shared as integer requests, calls
dim shared as TypedCallback static_typed = @typed_callback

function typed_callback cdecl(byval value as long, byval userdata as any ptr) as long
	calls += 1
	return value + *cptr(long ptr, userdata)
end function

function choose_callback() as TypedCallback
	requests += 1
	return static_typed
end function

sub run_values()
	dim payload as long = 25
	dim erased as ErasedCallback = cptr(ErasedCallback, static_typed)
	if callback_probe(erased, 17, @payload) <> 42 then end 31
	dim restored as TypedCallback = cptr(TypedCallback, erased)
	if restored(17, @payload) <> 42 then end 32
	if cptr(any ptr, erased) <> cptr(any ptr, static_typed) then end 33
	if cptr(any ptr, restored) <> cptr(any ptr, @typed_callback) then end 34
	if callback_probe(cptr(ErasedCallback, @typed_callback), 17, @payload) <> 42 then end 35
	if callback_probe(cptr(ErasedCallback, choose_callback()), 17, @payload) <> 42 then end 36
	if requests <> 1 then end 37
	dim gate as long = callback_gate()
	if gate andalso callback_probe(cptr(ErasedCallback, choose_callback()), 17, @payload) = 42 then end 38
	if requests <> 1 then end 39
	dim echoed as ErasedCallback = callback_echo(erased)
	if cptr(TypedCallback, echoed)(17, @payload) <> 42 then end 40
	for iteration as long = 1 to 64
		erased = cptr(ErasedCallback, choose_callback())
		restored = cptr(TypedCallback, erased)
		if restored(17, @payload) <> 42 then end 41
	next
	if requests <> 65 or calls <> 69 then end 42
	dim foreign_callback as TypedCallback = cptr(TypedCallback, callback_get())
	if foreign_callback(17, @payload) <> 52 then end 43
	dim empty_typed as TypedCallback = callback_null()
	dim empty_erased as ErasedCallback = cptr(ErasedCallback, empty_typed)
	if callback_is_null(empty_erased) <> 1 then end 44
	if cptr(TypedCallback, empty_erased) <> 0 then end 45
end sub

'' The C emitter ignores the ordinary x86 convention tags on other
'' CPU families. Exercise their stored addresses through the same foreign API.
#if not defined(__FB_X86__) or defined(__FB_64BIT__)
type StdCallback as function stdcall(byval as long, byval as any ptr) as long
declare function std_callback stdcall(byval value as long, byval userdata as any ptr) as long
extern "Windows-MS"
    type MsCallback as function(byval as long, byval as any ptr) as long
    declare function ms_callback(byval value as long, byval userdata as any ptr) as long
end extern
dim shared as integer convention_calls, convention_requests

function std_callback stdcall(byval value as long, byval userdata as any ptr) as long
    convention_calls += 1
    return value + *cptr(long ptr, userdata)
end function
extern "Windows-MS"
    function ms_callback(byval value as long, byval userdata as any ptr) as long
        convention_calls += 1
        return value + *cptr(long ptr, userdata) + 5
    end function
end extern

function choose_std_callback() as StdCallback
    convention_requests += 1
    return @std_callback
end function

sub run_conventions()
    dim payload as long = 25
    dim target as StdCallback = @std_callback
    dim erased as ErasedCallback = cptr(ErasedCallback, target)
    if callback_probe(erased, 17, @payload) <> 42 then end 51
    if cptr(StdCallback, callback_echo(erased))(17, @payload) <> 42 then end 52
    if cptr(any ptr, erased) <> cptr(any ptr, target) then end 53
    if callback_probe(cptr(ErasedCallback, @std_callback), 17, @payload) <> 42 then end 54
    if callback_probe(cptr(ErasedCallback, choose_std_callback()), 17, @payload) <> 42 then end 55
    dim gate as long = callback_gate()
    if gate andalso callback_probe(cptr(ErasedCallback, choose_std_callback()), 17, @payload) = 42 then end 56
    if convention_requests <> 1 then end 57
    for iteration as long = 1 to 64
        erased = cptr(ErasedCallback, choose_std_callback())
        target = cptr(StdCallback, erased)
        if target(17, @payload) <> 42 then end 58
    next
    dim ms_target as MsCallback = @ms_callback
    if callback_probe(cptr(ErasedCallback, ms_target), 17, @payload) <> 47 then end 59
    if convention_requests <> 65 or convention_calls <> 69 then end 60
    dim foreign_target as StdCallback = cptr(StdCallback, callback_get())
    if foreign_target(17, @payload) <> 52 then end 61
    dim empty_target as StdCallback = 0
    erased = cptr(ErasedCallback, empty_target)
    if callback_is_null(erased) <> 1 orelse cptr(StdCallback, erased) <> 0 then end 62
end sub
#endif

run_values()
#if not defined(__FB_X86__) or defined(__FB_64BIT__)
run_conventions()
#endif
'' The automated runner requires this single completion marker.
'' FB-LINTER: DISABLE-NEXT-LINE FBL012
print "CALLBACK_ERASURE_VALUES_PASS"

'' end of c-generic-callback-values.bas
