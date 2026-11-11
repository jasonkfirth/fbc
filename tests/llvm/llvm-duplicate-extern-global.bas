' TEST_MODE : COMPILE_AND_RUN_OK
'
' Project: FreeBASIC compiler regression tests
' File: tests/llvm/llvm-duplicate-extern-global.bas
' Purpose: Ensure multiple BASIC declarations for one C global emit one LLVM symbol.
' Responsibilities: Compare two BASIC aliases to one C-defined integer global.
' This file intentionally does NOT contain: LLVM backend or C runtime definitions.
'
#cmdline "-gen llvm"

Extern c2fb_first Alias "fb_llvm_duplicate_extern_global" As Long
Extern c2fb_second Alias "fb_llvm_duplicate_extern_global" As Long

If c2fb_first <> 42 Or c2fb_second <> 42 Then End 1
End 0

' end of llvm-duplicate-extern-global.bas
