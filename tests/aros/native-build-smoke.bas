'==============================================================================
'
' FreeBASIC AROS Native-Build Smoke Test
'
' File: native-build-smoke.bas
'
' Purpose:
'
'     Prove that an AROS-hosted FreeBASIC compiler can build an executable
'     which the same guest can load and run.
'
' Responsibilities:
'
'     - print a recognizable success line
'     - write a marker after the FreeBASIC runtime has initialized
'     - return a nonzero result that distinguishes execution from shell success
'
' This file intentionally does NOT contain:
'
'     - graphics or sound coverage
'     - compiler or toolchain setup
'     - emulator-specific behavior
'
'==============================================================================

#lang "fb"

Print "FREEBASIC_NATIVE_PROGRAM_OK"

Open "RAM:program.ok" For Output As #1
Print #1, "FREEBASIC_NATIVE_PROGRAM_EXECUTED"
Close #1

' A distinctive process result proves that the shell actually waited for the
' generated executable instead of merely finding a file with the same name.
End 73

'==============================================================================
' End of file
'==============================================================================
