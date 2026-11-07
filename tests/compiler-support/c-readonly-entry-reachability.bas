'' Project: FreeBASIC C backend regression tests
'' File: c-readonly-entry-reachability.bas
'' Purpose: Distinguish implementation calls from original procedure addresses.
'' Responsibilities: Check live/dead address edges, self addresses and shared statics.
'' This file intentionally does NOT depend on windows, files or external libraries.

'' Private CDECL carriers intentionally bypass read-only body separation.
'' Use the normal BASIC convention for the private entries under test.
type TextReader as function(byval as zstring ptr) as long

private function DirectRead(byval text as zstring ptr) as long
	return text[0]
end function

private function DeadAddress() as TextReader
	return @DirectRead
end function

private function AddressRead(byval text as zstring ptr) as long
	return text[0] + 1
end function

private function SelfAddressRead(byval text as zstring ptr) as long
	dim reader as TextReader = @SelfAddressRead
	if reader <> @SelfAddressRead then return -1
	return text[0] + 2
end function

public function SharedRead cdecl alias "readonly_shared_entry"(byval text as zstring ptr) as long
	static as long calls
	calls += 1
	return text[0] + calls
end function

#ifdef CHECK_OPAQUE
asm
	nop
end asm
#endif

if DirectRead("A") <> 65 then end 1
dim as TextReader reader = @AddressRead
if reader("A") <> 66 then end 2
if AddressRead("B") <> 67 then end 3
if SelfAddressRead("A") <> 67 then end 4
dim shared_reader as function cdecl(byval as zstring ptr) as long = @SharedRead
if SharedRead("A") <> 66 then end 5
if shared_reader("A") <> 67 then end 6
if SharedRead("A") <> 68 then end 7
end 0

'' end of c-readonly-entry-reachability.bas
