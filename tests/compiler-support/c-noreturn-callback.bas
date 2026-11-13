/'
Project: FreeBASIC C backend regression
File: c-noreturn-callback.bas
Purpose: Pass a terminating procedure through its matching callback signature.
Responsibilities: Check static initialization, returned values and address identity.
This file intentionally does NOT exercise signature or convention conversions.
'/
#lang "fb"

type Callback as sub cdecl(byval context as any ptr)

extern "C"
	declare function callback_addresses_equal(byval direct as Callback, _
		byval stored as Callback, byval returned as Callback) as long
	declare sub callback_invoke(byval value as Callback, byval context as any ptr)
end extern

sub finish cdecl(byval context as any ptr)
	if context = 0 then end 91
	'' The sentinel checks that C forwarded the original context pointer.
	if *cptr(long ptr, context) <> 4711 then end 92
	end 0
end sub

dim shared stored as Callback = @finish

function selected_callback() as Callback
	return @finish
end function

#ifdef NORETURN_CALLBACK_MISMATCH
sub different_signature cdecl(byval value as long)
	end value
end sub

'' Compile-only control: terminating bodies still require matching signatures.
dim mismatched as Callback = cptr(Callback, @different_signature)
callback_invoke(mismatched, 0)
#endif

if callback_addresses_equal(@finish, stored, selected_callback()) <> 1 then end 93
dim context as long = 4711
callback_invoke(@finish, @context)
end 94

'' end of c-noreturn-callback.bas
