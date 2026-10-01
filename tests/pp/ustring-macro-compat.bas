' TEST_MODE : COMPILE_AND_RUN_OK

'' Project: FreeBASIC compiler regression tests
'' File: ustring-macro-compat.bas
'' Purpose: Preserve the USTRING macro used by existing wide-string libraries.
'' Responsibilities: Exercise object-like macros, redefinition, and #undef.
'' This file intentionally does NOT contain native UTF-8 runtime operations.

type compatibility_string
	value as integer
end type

#define ustring compatibility_string
#define ustring compatibility_string
dim as ustring text
text.value = 123
if( text.value <> 123 ) then end 1

#undef ustring
#define ustring integer
dim as ustring number = 456
if( number <> 456 ) then end 2
#undef ustring

'' end of ustring-macro-compat.bas
