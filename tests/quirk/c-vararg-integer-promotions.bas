' TEST_MODE : COMPILE_AND_RUN_OK

'' Project: FreeBASIC C backend regression tests
'' File: c-vararg-integer-promotions.bas
'' Purpose: Check default C integer promotions in variadic calls.
'' Responsibilities: Cover signed and unsigned narrow values and 32-bit bounds.
'' This file intentionally does NOT depend on a graphics or audio device.
#include once "crt.bi"

private sub checkValues cdecl(byval sentinel as long, ...)
    dim as cva_list args = any
    cva_start(args, sentinel)
    if cva_arg(args, long) <> -128 then end 1
    if cva_arg(args, long) <> 255 then end 2
    if cva_arg(args, long) <> -32768 then end 3
    if cva_arg(args, long) <> 65535 then end 4
    if cva_arg(args, ulong) <> &hFFFFFFFFul then end 5
    cva_end(args)
end sub

dim as byte signedByte = -128
dim as ubyte unsignedByte = 255
dim as short signedShort = -32768
dim as ushort unsignedShort = 65535
dim as ulong unsignedLong = &hFFFFFFFFul
checkValues(0, signedByte, unsignedByte, signedShort, unsignedShort, unsignedLong)

sub checkFormat(byval unsignedByte as ubyte, byval unsignedShort as ushort, byval signedByte as byte, byval signedShort as short, byval unsignedLong as ulong)
    dim as zstring * 80 result
    sprintf(@result, "%hu %hu %hd %hd %u", unsignedByte, unsignedShort, signedByte, signedShort, unsignedLong)
    if result <> "255 65535 -128 -32768 4294967295" then end 6
end sub
checkFormat(unsignedByte, unsignedShort, signedByte, signedShort, unsignedLong)
print "C_VARARG_INTEGER_PROMOTIONS_PASS"
'' end of c-vararg-integer-promotions.bas
