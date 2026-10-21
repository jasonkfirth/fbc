'' Project: FreeBASIC test suite
'' File: resume.bas
'' Purpose: Verify FBLite ON ERROR, RESUME and RESUME NEXT behavior.
'' Responsibilities: Retry one failed statement and skip another.
'' This file intentionally does NOT contain: runtime error support in FB mode.
' TEST_MODE : COMPILE_AND_RUN_OK

#lang "fblite"
#cmdline "-ex"

Dim handlerCount As Integer
Dim nextReached As Integer
Dim outerReached As Integer
Dim retryFilename As String = "fbc-fblite-resume-retry.tmp"
Dim retryText As String

Private Function fbcTestsLocalErrorHandler() As Integer
	Dim localReached As Integer
	On Local Error Goto localHandler
	Error 25
	localReached = 1
	On Local Error Goto 0
	Function = localReached
	Exit Function

localHandler:
	Resume Next
End Function

If Dir(retryFilename) <> "" Then Kill retryFilename
On Error Goto retryHandler
Open retryFilename For Input As #1
On Error Goto 0
Line Input #1, retryText
Close #1
Kill retryFilename

If retryText <> "retry" Then End 1
If handlerCount <> 1 Then End 2
If Err <> 0 Then End 3

On Error Goto nextHandler
Error 24
nextReached = 42
On Error Goto 0

If nextReached <> 42 Then End 4
If Err <> 0 Then End 5
On Error Goto outerHandler
If fbcTestsLocalErrorHandler() <> 1 Then End 8
Error 26
outerReached = 1
On Error Goto 0
If outerReached <> 1 Then End 9
End 0

retryHandler:
handlerCount += 1
If Err = 0 Then End 6
Open retryFilename For Output As #2
Print #2, "retry"
Close #2
Resume

nextHandler:
If Err <> 24 Then End 7
Resume Next

outerHandler:
If Err <> 26 Then End 10
Resume Next

'' end of resume.bas
