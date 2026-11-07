'' Project: FreeBASIC compiler backend regression tests
'' File: thread-contextual-procedure.bas
'' Purpose: Keep Thread usable as a procedure name alongside THREAD LOCAL.
'' Responsibilities: Check declarations, callback addresses, overloads and live TLS isolation.
'' This file intentionally does NOT contain graphics or error-unwind concurrency tests.

#lang "fb"

Declare Sub Thread(ByVal context As Any Ptr)

Thread Local Dim Shared contextualThreadValue As Long = 7
Dim Shared contextualThreadMutex As Any Ptr
Dim Shared contextualThreadReady As Long
Dim Shared contextualThreadRelease As Long
Dim Shared contextualThreadInitial(0 To 1) As Long
Dim Shared contextualThreadResults(0 To 1) As Long

Namespace ContextualNames

	Declare Function Thread Overload(ByVal value As Integer) As Integer
	Declare Function Thread Overload(ByVal first As Integer, ByVal second As Integer) As Integer

End Namespace

Function ContextualNames.Thread(ByVal value As Integer) As Integer
	Return value + 1
End Function

Function ContextualNames.Thread(ByVal first As Integer, ByVal second As Integer) As Integer
	Return first + second
End Function

Sub Thread(ByVal context As Any Ptr)
	Dim As Integer workerIndex = *CPtr(Integer Ptr, context)
	If workerIndex < 0 OrElse workerIndex > 1 Then End 3
	contextualThreadInitial(workerIndex) = contextualThreadValue
	contextualThreadValue = workerIndex + 11

	MutexLock(contextualThreadMutex)
	contextualThreadReady += 1
	MutexUnlock(contextualThreadMutex)

	'' Bound coordination even when one worker cannot reach the barrier.
	Dim As Long released
	For poll As Integer = 1 To 500
		MutexLock(contextualThreadMutex)
		released = contextualThreadRelease
		MutexUnlock(contextualThreadMutex)
		If released <> 0 Then Exit For
		Sleep 1
	Next
	If released = 0 Then End 4
	contextualThreadResults(workerIndex) = contextualThreadValue
End Sub

If contextualThreadValue <> 7 Then End 5
contextualThreadValue = 91
contextualThreadMutex = MutexCreate()
If contextualThreadMutex = 0 Then End 1

Dim callback As Sub(ByVal context As Any Ptr) = @Thread
'' Payload storage remains live until both workers have been joined.
Dim As Integer workerIndices(0 To 1) = {0, 1}
Dim As Any Ptr firstThread = ThreadCreate(callback, @workerIndices(0))
Dim As Any Ptr secondThread = ThreadCreate(callback, @workerIndices(1))
Dim As Long expectedWorkers, readyWorkers
If firstThread <> 0 Then expectedWorkers += 1
If secondThread <> 0 Then expectedWorkers += 1

For poll As Integer = 1 To 500
	MutexLock(contextualThreadMutex)
	readyWorkers = contextualThreadReady
	MutexUnlock(contextualThreadMutex)
	If readyWorkers >= expectedWorkers Then Exit For
	Sleep 1
Next

'' Release and join any successfully created worker before checking failure.
MutexLock(contextualThreadMutex)
contextualThreadRelease = 1
MutexUnlock(contextualThreadMutex)
If firstThread <> 0 Then ThreadWait(firstThread)
If secondThread <> 0 Then ThreadWait(secondThread)
MutexDestroy(contextualThreadMutex)

If expectedWorkers <> 2 OrElse readyWorkers <> 2 Then End 2
If contextualThreadInitial(0) <> 7 OrElse contextualThreadInitial(1) <> 7 Then End 6
If contextualThreadResults(0) <> 11 OrElse contextualThreadResults(1) <> 12 Then End 7
If contextualThreadValue <> 91 Then End 8
If ContextualNames.Thread(4) <> 5 Then End 9
If ContextualNames.Thread(4, 6) <> 10 Then End 10
Print "CONTEXTUAL_THREAD_PROCEDURE_PASS"

'' end of thread-contextual-procedure.bas
