'' Project: FreeBASIC compiler backend regression tests
'' -----------------------------------------
''
'' File: thread-local-extern.bas
''
'' Purpose:
''
''     Verify C thread-local variables keep their C ABI and per-thread value.
''
'' Responsibilities:
''
''     - read and update a C-defined thread-local variable from worker threads
''     - confirm the main thread retains its own initialized value
''
'' This file intentionally does NOT contain:
''
''     - the C thread-local definition or compiler backend expectations
''

#lang "fb"

Extern "C"
	Thread Local Extern externalThreadLocalValue Alias "externalThreadLocalValue" As ULong
	Thread Local Extern externalThreadLocalValues(0 To 1) Alias "externalThreadLocalValues" As ULong
End Extern

Dim Shared externalThreadLocalResults(0 To 1) As Long
Dim Shared externalThreadLocalArrayResults(0 To 1) As Long
Dim Shared externalThreadLocalFailure(0 To 1) As Long

Private Sub ThreadLocalExternWorker(ByVal context As Any Ptr)
	Dim As Long workerIndex = CInt(context)
	If externalThreadLocalValue <> 7 OrElse _
	   externalThreadLocalValues(0) <> 3 OrElse _
	   externalThreadLocalValues(1) <> 4 Then
		externalThreadLocalFailure(workerIndex) = 1
		Return
	End If
	externalThreadLocalValue = workerIndex + 10
	externalThreadLocalValues(workerIndex) = workerIndex + 20
	externalThreadLocalResults(workerIndex) = externalThreadLocalValue
	externalThreadLocalArrayResults(workerIndex) = externalThreadLocalValues(workerIndex)
End Sub

Dim As Any Ptr firstThread = ThreadCreate(@ThreadLocalExternWorker, Cast(Any Ptr, 0))
Dim As Any Ptr secondThread = ThreadCreate(@ThreadLocalExternWorker, Cast(Any Ptr, 1))
If firstThread = 0 OrElse secondThread = 0 Then End 2
ThreadWait(firstThread)
ThreadWait(secondThread)

If externalThreadLocalFailure(0) <> 0 OrElse _
   externalThreadLocalFailure(1) <> 0 Then End 9
If externalThreadLocalResults(0) <> 10 Then End 3
If externalThreadLocalResults(1) <> 11 Then End 4
If externalThreadLocalArrayResults(0) <> 20 Then End 6
If externalThreadLocalArrayResults(1) <> 21 Then End 7
If externalThreadLocalValue <> 7 Then End 5
If externalThreadLocalValues(0) <> 3 OrElse externalThreadLocalValues(1) <> 4 Then End 8

Print "thread-local external passed"

'' end of thread-local-extern.bas
