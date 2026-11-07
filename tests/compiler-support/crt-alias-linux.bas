/'
    Project: FreeBASIC compiler regression tests
    File: crt-alias-linux.bas
    Purpose: Check object names for reserved CRT-style declarations on Linux.
    Responsibilities: Call four externally supplied conversion functions.
    This fixture does not implement the conversion functions.
'/
extern "C"
    declare function _strtoi64 cdecl ( byval text_ as const zstring ptr, byval end_ as zstring ptr ptr, _
                                      byval radix_ as long ) as longint
    declare function _strtoui64 cdecl ( byval text_ as const zstring ptr, byval end_ as zstring ptr ptr, _
                                        byval radix_ as long ) as ulongint
    declare function _wcstoi64 cdecl ( byval text_ as const wstring ptr, byval end_ as wstring ptr ptr, _
                                      byval radix_ as long ) as longint
    declare function _wcstoui64 cdecl ( byval text_ as const wstring ptr, byval end_ as wstring ptr ptr, _
                                        byval radix_ as long ) as ulongint
end extern
dim as wstring * 2 wide_ = wstr("x")
if _strtoi64(@"x", 0, 10) <> -101 then end 1
if _strtoui64(@"x", 0, 10) <> 102 then end 2
if _wcstoi64(@wide_, 0, 10) <> -103 then end 3
if _wcstoui64(@wide_, 0, 10) <> 104 then end 4
print "CRT_ALIAS_LINUX_PASS"

/' end of crt-alias-linux.bas '/
