'' Project: FreeBASIC Windows import regression
'' File: import-data.bas
'' Purpose: Read and update actual C DLL data with the IMPORT attribute.
'' Responsibilities: Exact C widths, address indirection and cross-language writes.
'' This file contains no replacement DLL or platform import-name workaround.

#lang "fb"
#inclib "import-data"

Type GuardedValue
	before_value As Long
	value As Long
	after_value As Long
End Type

Extern "C"
	Extern Import scalar_value Alias "import_scalar" As Long
	Extern Import values(0 To 2) Alias "import_values" As Long
	Extern Import value_pointer Alias "import_pointer" As Long Ptr
	Extern Import wide_value Alias "import_wide" As ULongInt
	Extern Import record_value Alias "import_record" As GuardedValue
	Declare Function native_address Alias "import_address"() As Long Ptr
	Declare Function native_verify Alias "import_verify"() As Long
	Declare Sub native_change Alias "import_change"()
End Extern

If scalar_value <> -2147483000 Then End 1
If values(0) <> 17 OrElse values(1) <> -9 OrElse values(2) <> &h1234567 Then End 2
If value_pointer <> @values(1) OrElse *value_pointer <> -9 Then End 3
If wide_value <> &hFEDCBA9876543210ULL Then End 4
If record_value.value <> -7 Then End 5
If native_address() <> @scalar_value Then End 6

scalar_value = 12345
values(0) = -123
*value_pointer = 42
value_pointer = @values(2)
wide_value = &h8000000000000001ULL
record_value.value = -99
If native_verify() <> 1 Then End 7

'' The DLL must observe our writes, and its writes must remain visible through
'' the same import addresses. The guards catch accidental native-width stores.
native_change()
If scalar_value <> -333 OrElse values(0) <> 888 OrElse record_value.value <> 555 Then End 8
If record_value.before_value <> &h13579BDF OrElse record_value.after_value <> &h2468ACE Then End 9
Print "DLL_IMPORT_DATA=PASS"

'' end of import-data.bas
