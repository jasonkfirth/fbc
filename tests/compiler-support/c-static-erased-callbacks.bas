'' Project: FreeBASIC C backend tests
'' File: c-static-erased-callbacks.bas
'' Purpose: Retain static code addresses in native ANY PTR callback slots.
'' Responsibilities: Exercise static relocation and restored typed call ABIs.
'' This file intentionally does NOT contain graphics or input handling.

#lang "fb"

'' This fixture owns the counter and updates it through the stored callback.
dim shared as integer callback_total

private function addValue cdecl(byval value as long) as long
	return value + 1
end function

private function valueAddress(byref value as integer) as integer ptr
	return @value
end function

private sub accumulate stdcall(byval value as integer)
	callback_total += value
end sub

'' These slots erase signatures for storage. Each caller restores the exact
'' original type and convention before invoking its selected address.
dim shared as any ptr callback_slots(0 to 3) = {@addValue, @valueAddress, @accumulate, 0}

dim as integer value = 41
if cptr(function cdecl(byval as long) as long, callback_slots(0))(40) <> 41 then end 1
if cptr(function(byref as integer) as integer ptr, callback_slots(1))(value) <> @value then end 2
cptr(sub stdcall(byval as integer), callback_slots(2))(value)
if callback_total <> 41 orelse callback_slots(3) <> 0 then end 3
print "C_STATIC_ERASED_CALLBACKS_PASS"

'' end of c-static-erased-callbacks.bas
