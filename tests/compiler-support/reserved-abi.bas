/'
    Project: FreeBASIC compiler regression tests
    File: reserved-abi.bas
    Purpose: Check C identifier spelling without changing external symbols.
    Responsibilities: Call an external function, access a global and a field.
    This fixture does not define its external C symbols.
'/
type field_record
    __private_value as long
end type

extern "C"
    declare function _fb_test_external_add cdecl ( byval value as long ) as long
    declare function fb_test_inspect_field cdecl ( byval value as const field_record ptr ) as long
    declare function bit_count cdecl alias "__builtin_popcount" ( byval value as ulong ) as long
#ifdef __FB_WIN32__
    declare function _strtoi64 cdecl ( byval text_ as const zstring ptr, byval end_ as zstring ptr ptr, _
                                      byval radix_ as long ) as longint
    declare function _strtoui64 cdecl ( byval text_ as const zstring ptr, byval end_ as zstring ptr ptr, _
                                        byval radix_ as long ) as ulongint
    declare function _wcstoi64 cdecl ( byval text_ as const wstring ptr, byval end_ as wstring ptr ptr, _
                                      byval radix_ as long ) as longint
    declare function _wcstoui64 cdecl ( byval text_ as const wstring ptr, byval end_ as wstring ptr ptr, _
                                        byval radix_ as long ) as ulongint
#endif
    extern external_value alias "__fb_test_external_value" as long
end extern

dim as field_record item
item.__private_value = 3
dim as long _ordinary_local = 7
dim as long __double_local = 11
if _fb_test_external_add(2) <> 7 then end 1
if external_value <> 17 then end 2
if bit_count(&hb4) <> 4 then end 3
if item.__private_value + _ordinary_local + __double_local <> 21 then end 4
if fb_test_inspect_field(@item) <> 3 then end 9
#ifdef __FB_WIN32__
if _strtoi64(@"1234567890123", 0, 10) <> 1234567890123LL then end 5
if _strtoui64(@"18446744073709551615", 0, 10) <> &hffffffffffffffffull then end 6
dim as wstring * 24 wide_signed = wstr("-1234567890123")
dim as wstring * 24 wide_unsigned = wstr("18446744073709551615")
if _wcstoi64(@wide_signed, 0, 10) <> -1234567890123LL then end 7
if _wcstoui64(@wide_unsigned, 0, 10) <> &hffffffffffffffffull then end 8
#endif
print "RESERVED_ABI_TEST_PASS"

/' end of reserved-abi.bas '/
