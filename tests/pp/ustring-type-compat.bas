' TEST_MODE : COMPILE_AND_RUN_OK

'' Project: FreeBASIC compiler regression tests
'' File: ustring-type-compat.bas
'' Purpose: Preserve legacy UString type names with FB_NO_USTRING.
'' Responsibilities: Check forward aliases, legacy STRING aliases, and restoration.
'' This file intentionally does NOT contain: native UTF-8 implementation details.

#define FB_NO_USTRING

namespace legacy

	private type UStr as UString
	private type UString
		value as integer
	end type

	function check_value( ) as integer
		dim as UStr text
		text.value = 123
		return text.value
	end function

end namespace

if( legacy.check_value() <> 123 ) then end 1

'' OHR's configuration header uses this alias for translated source files.
type USTRING as STRING
dim as USTRING message = "legacy"
if( len(message) <> 6 ) then end 2

#undef FB_NO_USTRING

'' Removing the opt-out restores the built-in type; the namespaced user
'' declaration must not change subsequent native USTRING declarations.
#assert sizeof(UString) = sizeof(string)
dim as UString native_text = uchr(&hE9)
if( len(native_text) <> 1 ) then end 3

'' end of ustring-type-compat.bas
