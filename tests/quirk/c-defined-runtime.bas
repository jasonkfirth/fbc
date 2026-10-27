'' TEST_MODE : COMPILE_AND_RUN_OK
'' Project: FreeBASIC compiler regression tests
'' File: c-defined-runtime.bas
'' Purpose: Verify typed runtime dispatch after rejecting unsupported paths.
'' Responsibilities: Exercise INPUT, PRINT, WRITE, raw string descriptors and
''     string-to-number overloads with the supported scalar widths.
'' This file intentionally does NOT access printers or external data files.
#lang "fb"

dim as boolean flag_value
dim as byte byte_value
dim as ubyte ubyte_value
dim as short short_value
dim as ushort ushort_value
dim as long long_value
dim as ulong ulong_value
dim as integer integer_value
dim as uinteger uinteger_value
dim as longint longint_value
dim as ulongint ulongint_value
dim as single single_value
dim as double double_value
dim as string string_value
dim as string * 16 fixed_value
dim as zstring * 16 zero_value
dim as wstring * 16 wide_value
dim as ustring unicode_value

'' The file belongs to this test's private working directory.
const data_file = "c-defined-runtime.tmp"
open data_file for output as #1
write #1, true, cbyte(-7), cubyte(250), cshort(-31000), cushort(62000)
write #1, clng(-123456), culng(4000000000ul), cint(-4567), cuint(4567)
write #1, -5000000000ll, 10000000000ull, csng(1.25), 2.5
write #1, "dynamic", "fixed", "zero", "wide", "unicode"
close #1
open data_file for input as #1
input #1, flag_value, byte_value, ubyte_value, short_value, ushort_value
input #1, long_value, ulong_value, integer_value, uinteger_value
input #1, longint_value, ulongint_value, single_value, double_value
input #1, string_value, fixed_value, zero_value, wide_value, unicode_value
close #1
if( flag_value <> true ) then end 1
if( byte_value <> -7 or ubyte_value <> 250 ) then end 2
if( short_value <> -31000 or ushort_value <> 62000 ) then end 3
if( long_value <> -123456 or ulong_value <> 4000000000ul ) then end 4
if( integer_value <> -4567 or uinteger_value <> 4567 ) then end 5
if( longint_value <> -5000000000ll or ulongint_value <> 10000000000ull ) then end 6
if( abs(single_value - 1.25) > 0.0001 ) then end 7
if( abs(double_value - 2.5) > 0.0001 ) then end 8
if( string_value <> "dynamic" ) then end 9
if( rtrim(fixed_value) <> "fixed" or zero_value <> "zero" ) then end 10
if( wide_value <> wstr("wide") or unicode_value <> ustring("unicode") ) then end 11

'' Exercise PRINT independently of WRITE's quoting and INPUT's argument counts.
open data_file for output as #1
print #1, flag_value; byte_value; ubyte_value; short_value; ushort_value
print #1, long_value; ulong_value; integer_value; uinteger_value
print #1, longint_value; ulongint_value; single_value; double_value
print #1, string_value; fixed_value; zero_value; wide_value; unicode_value
close #1

'' Raw fixed/literal storage needs a temporary read-only STRING descriptor.
fixed_value = "314"
zero_value = "271"
wide_value = wstr("19")
unicode_value = ustring("23")
if( valint(fixed_value) <> 314 or valint("123") <> 123 ) then end 12
if( cint(zero_value) <> 271 or clngint(fixed_value) <> 314 ) then end 13
if( cuint("429") <> 429 or culngint("10000000000") <> 10000000000ull ) then end 14
if( clng(wide_value) <> 19 or clng(unicode_value) <> 23 ) then end 15
if( abs(cdbl("1.5") - 1.5) > 0.0001 ) then end 16
kill data_file
print "DEFINED_RUNTIME_PASS"
end 0
'' end of c-defined-runtime.bas
