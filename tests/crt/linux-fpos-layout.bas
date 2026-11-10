' TEST_MODE : COMPILE_AND_RUN_OK
' Project: FreeBASIC Linux CRT tests
' File: linux-fpos-layout.bas
' Purpose: Keep fpos_t compatible with glibc's large-file ABI.
' Responsibilities: Check the file position and conversion-state offsets.
' This file intentionally does NOT contain: File I/O behavior tests.

#if defined(__FB_LINUX__)
#include once "crt/stdio.bi"

Dim position As fpos_t

If SizeOf(fpos_t) <> 16 Then End 1
If Cast(ULongInt, @position.__pos) - Cast(ULongInt, @position) <> 0 Then End 2
If Cast(ULongInt, @position.__state) - Cast(ULongInt, @position) <> 8 Then End 3
#endif

' end of linux-fpos-layout.bas
