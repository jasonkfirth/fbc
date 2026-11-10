' TEST_MODE : COMPILE_AND_RUN_OK
'
' Project: FreeBASIC compiler regression tests
' File: llvm-duplicate-extern-global.bas
' Purpose: Ensure multiple BASIC declarations for one C global emit one LLVM symbol.
' Responsibilities: Compare the CRT stdout declaration with a C alias.
' This file intentionally does NOT contain: LLVM backend implementation code.
'
#cmdline "-gen llvm"
#include once "crt/stdio.bi"

Extern c2fb_stdout Alias "stdout" As FILE Ptr

If c2fb_stdout <> stdout Then End 1
End 0

' end of llvm-duplicate-extern-global.bas
