/'
    Project: FreeBASIC C backend regression tests
    File: clong-promotion.bas
    Purpose: Keep arithmetic promotions independent of C long type aliases.
    Responsibilities: Check promoted signed and unsigned values and shifts.
    This file intentionally does NOT require an external C library.
'/
' TEST_MODE : COMPILE_AND_RUN_OK

#lang "fb"
#include once "crt/long.bi"

'' Windows has 32bit C long and 64bit INTEGER. Other targets still compile
'' this fixture, but do not need the widening operations checked here.
#if sizeof( integer ) > sizeof( clong )

dim as clong signed_minimum = -2147483648ll
dim as clong signed_maximum = 2147483647ll
dim as clong signed_unit = 1
dim as culong unsigned_maximum = &hffffffffull
dim as culong unsigned_unit = 1
dim as culong unsigned_two = 2

if( signed_minimum - signed_unit <> -2147483649ll ) then end 1
if( signed_maximum + signed_unit <> 2147483648ll ) then end 2
if( (signed_minimum - signed_unit) \ 64 <> -33554432ll ) then end 3
if( unsigned_maximum + unsigned_unit <> 4294967296ll ) then end 4
if( unsigned_maximum * unsigned_two <> 8589934590ll ) then end 5
if( (unsigned_maximum shl unsigned_unit) <> 8589934590ll ) then end 6

#endif

end 0

' end of clong-promotion.bas
