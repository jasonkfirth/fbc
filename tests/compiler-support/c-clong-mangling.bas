'' FreeBASIC C++ ABI regression, c-clong-mangling.bas
'' Checks the long and unsigned long codes of explicit C integer aliases.
'' Exercises signed values and the target's native C long width.
'' The companion C object supplies independently spelled Itanium ABI names.
#lang "fb"
#include once "crt/long.bi"

namespace CPPABI
	extern "C++"
		declare function CHECK_LONG cdecl(byval value as clong) as clong
		declare function CHECK_ULONG cdecl(byval value as culong) as culong
	end extern
end namespace

dim as clong signed_value = -19
dim as culong unsigned_value = &h12345678ul
#if sizeof(clong) = 8
	signed_value = cast(clong, cast(ulongint, 1) shl 40)
	unsigned_value = cast(culong, cast(ulongint, 1) shl 41)
#endif
if CPPABI.CHECK_LONG(signed_value) <> signed_value + 3 then end 1
if CPPABI.CHECK_LONG(-19) <> -16 then end 2
if CPPABI.CHECK_ULONG(unsigned_value) <> (unsigned_value xor cast(culong, &h55)) then end 3
print "CLONG_MANGLING_PASS"

'' end of c-clong-mangling.bas
