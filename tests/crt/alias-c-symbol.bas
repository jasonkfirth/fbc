'' Project: FreeBASIC compiler ABI tests
'' -----------------------------------------
''
'' File: alias-c-symbol.bas
''
'' Purpose:
''
''     Check typed BASIC declarations sharing one external C symbol.
''
'' Responsibilities:
''
''     - require separate C identifiers for incompatible pointer declarations
''     - verify that calls and procedure addresses keep the external ABI name
''
'' This file intentionally does NOT contain:
''
''     - dependencies on external libraries
''

#include "fbcunit.bi"

extern "c"
	function fbc_test_alias_identity cdecl (byval value as any ptr) as any ptr
		return value
	end function

	declare function integer_identity cdecl alias "fbc_test_alias_identity" _
		(byval value as integer ptr) as integer ptr
	declare function double_identity cdecl alias "fbc_test_alias_identity" _
		(byval value as double ptr) as double ptr
end extern

SUITE( fbc_tests.crt.alias_c_symbol )
	TEST( typed_aliases )
		dim as integer i = 123
		dim as double d = 4.5
		CU_ASSERT( integer_identity(@i) = @i )
		CU_ASSERT( double_identity(@d) = @d )
		dim as function cdecl (byval as integer ptr) as integer ptr fn = @integer_identity
		CU_ASSERT( fn(@i) = @i )
	END_TEST
END_SUITE

'' end of alias-c-symbol.bas
