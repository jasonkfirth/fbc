/'
    Project: FreeBASIC console qualification
    File: emulator-c-format.bas
    Purpose: Compare BASIC variadic calls with an independent native C baseline.
    Responsibilities: Report the actual outputs and require the standard result.
    This file does not accept a shared SDK failure as a formatting pass.
'/
#lang "fb"
#include once "crt/stdio.bi"

declare function fbqual_sprintf cdecl alias "fbqual_sprintf"(byval output as zstring ptr, byval capacity as ulong) as long
dim native_text as zstring * 128
dim basic_text as zstring * 128
dim native_count as long = fbqual_sprintf(@native_text, 128)
dim basic_count as long = sprintf(@basic_text, "%d %u %.2f %s", clng(-123), culng(4000000000), cdbl(1.25), strptr("text"))
#ifdef __FB_XBOX__
    const report_file = "E:\FBQCFMT.TXT"
#else
    const report_file = "FBQCFMT.TXT"
#endif
dim handle as integer = freefile
if open(report_file for output as #handle) <> 0 then end 61
print #handle, "native count: "; native_count
print #handle, "native text: "; native_text
print #handle, "BASIC count: "; basic_count
print #handle, "BASIC text: "; basic_text
close #handle
if native_count <> basic_count or native_text <> basic_text then end 62
if native_text <> "-123 4000000000 1.25 text" then end 63
if native_count <> len("-123 4000000000 1.25 text") then end 64
end 0

'' end of emulator-c-format.bas
