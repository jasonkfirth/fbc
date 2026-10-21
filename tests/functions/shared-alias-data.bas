'' Project: FreeBASIC compiler tests
'' File: shared-alias-data.bas
'' Purpose: Define shared storage with a C linker symbol.
'' Responsibilities: Check initialization, C reads, C writes, and FB reads.
'' This file intentionally does NOT test imported data declarations.

' TEST_MODE : MULTI_MODULE_OK

Dim Shared c2fb_shared_value Alias "c2fb_shared_alias_data" As Long = 17

Extern "C"
    Declare Function c2fb_read_shared_value Cdecl Alias "c2fb_read_shared_alias_data"() As Long
    Declare Sub c2fb_write_shared_value Cdecl Alias "c2fb_write_shared_alias_data"(ByVal value As Long)
End Extern

If c2fb_read_shared_value() <> 17 Then End 1
c2fb_write_shared_value(29)
If c2fb_shared_value <> 29 Then End 2
c2fb_shared_value = -43
If c2fb_read_shared_value() <> -43 Then End 3

Print "SHARED_ALIAS_DATA=PASS"

'' end of shared-alias-data.bas
