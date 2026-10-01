'' Project: FreeBASIC compiler tests
'' File: string/rtl-string-descriptor-invalid.bas
'' Purpose: Reject an incompatible literal string descriptor declaration.
'' Responsibilities: Exercise argument conversion failure without a null dereference.
'' This file intentionally does NOT contain: executable runtime helper replacements.

' TEST_MODE : COMPILE_ONLY_FAIL

'' Runtime headers can replace these declarations after removing the builtin.
'' A literal must be rejected when the replacement expects an integer address.
#undef fb_StrAllocTempDescZEx
declare function fb_StrAllocTempDescZEx _
	( byval bytes as integer, byval length as integer ) as any ptr

kill "name"

'' end of string/rtl-string-descriptor-invalid.bas
