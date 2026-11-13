'' Project: FreeBASIC compiler backend regression tests
'' File: thread-local-array-consumer.bas
'' Purpose: Access another module's initialized TLS scalar and fixed array.
'' Responsibilities: Check first-touch initialization, byte bias and thread isolation.
'' This file intentionally does NOT import external C TLS or use dynamic arrays.

#lang "fb"

Thread Local Extern externalValue As Long
Thread Local Extern externalGrid(-2 To -1, 3 To 4) As Long
Declare Function externalRead() As Long

Private Sub worker(ByVal context As Any Ptr)
	If context = 0 Then End 91
	Dim failure As Long Ptr = context
	'' Read the extern before the provider touches it in this thread.
	If externalValue <> 17 OrElse externalGrid(-2, 3) <> 3 OrElse externalGrid(-1, 4) <> 6 Then
		*failure = 1
		Exit Sub
	End If
	If externalRead() <> 20 Then
		*failure = 2
		Exit Sub
	End If
	externalValue = 33
	externalGrid(-2, 3) = 23
	If externalRead() <> 56 Then *failure = 3
End Sub

If externalValue <> 17 OrElse externalGrid(-2, 3) <> 3 OrElse externalGrid(-1, 4) <> 6 Then End 1
externalValue = 100
externalGrid(-2, 3) = 10
If externalRead() <> 110 Then End 2

Dim failure As Long
Dim handle As Any Ptr = ThreadCreate(@worker, @failure)
If handle = 0 Then End 3
ThreadWait(handle)
If failure <> 0 Then End 4
If externalValue <> 100 OrElse externalGrid(-2, 3) <> 10 Then End 5
If externalGrid(-1, 4) <> 6 OrElse externalRead() <> 110 Then End 6
Print "THREAD_LOCAL_ARRAY_MODULES_PASS"

'' end of thread-local-array-consumer.bas
