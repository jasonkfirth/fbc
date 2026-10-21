'' Project: FreeBASIC external library checks
'' File: gettext-width.bas
'' Purpose: Check gettext plural counts across the native C unsigned long width.
'' Responsibilities: Validate all plural APIs without requiring a message catalog.
'' This file contains no translated catalog, locale installation or GUI code.
#lang "fb"
#include once "libintl.bi"

private function matches(byval message as const zstring ptr, byref expected as const string) as boolean
    if message = 0 then return false
    return *message = expected
end function

dim count as culong = 2
#if sizeof(culong) = 8
    '' A truncated 64-bit count becomes one and incorrectly selects singular.
    count = cast(culong, &h100000001ull)
#endif

if not matches(ngettext("one", "many", 1), "one") then end 1
if not matches(ngettext("one", "many", count), "many") then end 2
if not matches(dngettext("freebasic-abi-check", "one", "many", count), "many") then end 3
if not matches(dcngettext("freebasic-abi-check", "one", "many", count, LC_ALL), "many") then end 4
print "GETTEXT_UNSIGNED_LONG_PASS bytes="; sizeof(culong)

'' end of gettext-width.bas
