'' Project: FreeBASIC compiler tests
'' File: string/rtl-fixed-descriptor-invalid.bas
'' Purpose: Reject an incompatible fixed string descriptor declaration.
'' Responsibilities: Exercise argument conversion failure without a null dereference.
'' This file intentionally does NOT contain: executable runtime helper replacements.

' TEST_MODE : COMPILE_ONLY_FAIL

'' A fixed-length buffer also needs a temporary descriptor for KILL's argument.
'' The invalid replacement must produce a diagnostic and stop that conversion.
#undef fb_StrAllocTempDescF
declare function fb_StrAllocTempDescF _
	( byval bytes as integer, byval length as integer ) as any ptr

dim filename as string * 8
kill filename

'' end of string/rtl-fixed-descriptor-invalid.bas
