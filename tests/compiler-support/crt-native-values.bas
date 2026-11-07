'' Project: FreeBASIC CRT contract regression tests
'' File: crt-native-values.bas
'' Purpose: Compare Windows CRT bindings with the native SDK and real CRT calls.
'' Responsibilities: Check stream layout, accessor types, values and owned I/O.
'' This file intentionally does NOT inspect private CRT stream state.
#lang "fb"
#include once "crt.bi"

type FileHolder
	lead as ubyte
	stream as FILE
end type

extern "C"
	declare function fb_crt_file_size() as size_t
	declare function fb_crt_file_offset() as size_t
	declare function fb_crt_stream(byval stream_index as ulong) as any ptr
	declare function fb_crt_errno(byval slot as long ptr, byval expected as long) as long
	declare function fb_crt_append(byval stream as any ptr) as long
end extern

#ifdef __FB_64BIT__
type StreamAccessor as function cdecl(byval as ulong) as FILE ptr
dim getter as StreamAccessor = procptr(__acrt_iob_func)
if sizeof(FILE) <> fb_crt_file_size() then end 1
dim holder as FileHolder
if cuint(@holder.stream) - cuint(@holder) <> fb_crt_file_offset() then end 2
for stream_index as ulong = 0 to 2
	if cptr(any ptr, getter(stream_index)) <> fb_crt_stream(stream_index) then end 3
next
#else
'' The 32-bit binding retains the legacy MSVCRT public record and _iob table.
#assert sizeof(FILE) = 32
#endif

dim errno_slot as long ptr = _errno()
if errno_slot = 0 then end 4
dim saved_errno as long = *errno_slot
*errno_slot = 37
if fb_crt_errno(errno_slot, 37) <> 1 then end 5
*errno_slot = saved_errno

dim duplicate as zstring ptr = _strdup("CRT shared values")
if duplicate = 0 then end 6
dim duplicate_ok as integer = (*duplicate = "CRT shared values")
free(duplicate)
if duplicate_ok = 0 then end 7
if _stricmp("AbCd", "aBcD") <> 0 then end 8
if _strnicmp("AbCdX", "aBcDY", 4) <> 0 then end 9

dim narrow_text as zstring * 64
if _itoa(-37, @narrow_text, 10) <> @narrow_text then end 10
if narrow_text <> "-37" then end 11
_ultoa(4294967295ul, @narrow_text, 10)
if narrow_text <> "4294967295" then end 12
_i64toa(-1234567890123ll, @narrow_text, 10)
if narrow_text <> "-1234567890123" then end 13
_ui64toa(&hffffffffffffffffull, @narrow_text, 10)
if narrow_text <> "18446744073709551615" then end 14

dim wide_text as wstring * 64
_itow(-37, @wide_text, 10)
if wide_text <> wstr("-37") then end 15
_ultow(4294967295ul, @wide_text, 10)
if wide_text <> wstr("4294967295") then end 16
_i64tow(-1234567890123ll, @wide_text, 10)
if wide_text <> wstr("-1234567890123") then end 17
_ui64tow(&hffffffffffffffffull, @wide_text, 10)
if wide_text <> wstr("18446744073709551615") then end 18

'' The controller supplies one reusable file in its retained work directory.
dim filename as string = command(1)
if len(filename) = 0 then end 19
dim stream as FILE ptr = fopen(strptr(filename), "w+b")
if stream = 0 then end 20
if _access(strptr(filename), 0) <> 0 then end 21
if fwrite(@"B", 1, 1, stream) <> 1 then end 22
if fb_crt_append(stream) <> 0 then end 23
if fflush(stream) <> 0 then end 24
if fseek(stream, 0, SEEK_SET) <> 0 then end 25
dim received as zstring * 8
if fread(@received, 1, 2, stream) <> 2 then end 26
received[2] = 0
if received <> "BC" then end 27
if fclose(stream) <> 0 then end 28
kill filename
print "CRT_NATIVE_VALUES_PASS"

'' end of crt-native-values.bas
