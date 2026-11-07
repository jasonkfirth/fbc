/'
Project: FreeBASIC C error-owner qualification
File: c-error-capture-layout.bas
Purpose: Keep mixed captures live across repeated ERROR recovery.
Responsibilities: Check values, references, managed fields and wide boundaries.
This file intentionally does NOT depend on private capture field offsets.
'/
#lang "fblite"

type PackedCapture field = 1
    tag as ubyte
    number as longint
end type

type ManagedCapture
    text as string
    tag as ubyte
end type

private function mixed_owner(byref visits as integer, byval seed as integer) as integer
    on local error goto caught
    dim small as ubyte = 7
    dim large as longint = 1234567890123ll
    dim narrow as ushort = 513
    dim text as string = "capture"
    dim packed as PackedCapture
    dim fraction as double = 1.5
    dim elements(-1 to 1) as ushort = {3, 5, 8}
    dim managed as ManagedCapture
    dim tail as byte = -3
    packed.tag = 11
    packed.number = large
    managed.text = "owned"
    managed.tag = 19
    error 24
    if visits <> 1 or text <> "capture!" then end 71
    error 25
    if visits <> 2 or text <> "capture!!" then end 72
    return seed + small + narrow + tail + elements(0)
caught:
    if err <> 24 and err <> 25 then end 73
    if small <> 7 or large <> 1234567890123ll then end 74
    if narrow <> 513 or fraction <> 1.5 then end 75
    if packed.tag <> 11 or packed.number <> large then end 76
    if elements(-1) <> 3 or elements(0) <> 5 or elements(1) <> 8 then end 77
    if managed.text <> "owned" or managed.tag <> 19 or tail <> -3 then end 78
    text += "!"
    visits += 1
    resume next
end function

private function wide_owner() as integer
    on local error goto caught
    dim first as ubyte = 2
    dim wide as integer<128> = 12345
    dim last as ushort = 9
    dim text as string = "wide"
    error 24
    return first + last
caught:
    if err <> 24 then end 81
    if first <> 2 or wide <> 12345 or last <> 9 or text <> "wide" then end 82
    wide += 1
    resume next
end function

dim visits as integer
if mixed_owner(visits, 10) <> 532 then end 83
if visits <> 2 then end 84
if wide_owner() <> 11 then end 85
print "C_ERROR_CAPTURE_LAYOUT_PASS"

'' end of c-error-capture-layout.bas
