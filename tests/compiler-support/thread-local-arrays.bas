'' Project: FreeBASIC compiler backend regression tests
'' File: thread-local-arrays.bas
'' Purpose: Index fixed arrays backed by native or runtime thread-local storage.
'' Responsibilities: Check byte bias, index evaluation and worker isolation.
'' This file intentionally does NOT use external C TLS or dynamic arrays.

#lang "fb"

Thread Local Dim Shared values(-2 To -1, 3 To 4) As Long = {{11, 12}, {21, 22}}
Thread Local Dim Shared indexCalls As Long

Private Function selectedRow() As Integer
	indexCalls += 1
	Return -2
End Function

Private Function selectedColumn() As Integer
	indexCalls += 1
	Return 3
End Function

Private Sub worker(ByVal context As Any Ptr)
	If context = 0 Then End 91
	Dim failure As Long Ptr = context
	If values(-2, 3) <> 11 OrElse values(-1, 4) <> 22 Then
		*failure = 1
		Exit Sub
	End If
	values(selectedRow(), selectedColumn()) = 200
	If indexCalls <> 2 OrElse values(-2, 3) <> 200 Then *failure = 2
End Sub

If Command(1) = "bounds" Then
	Dim invalidRow As Integer = -3
	Print values(invalidRow, 3)
	End 92
End If

If values(-2, 3) <> 11 OrElse values(-1, 4) <> 22 Then End 1
values(selectedRow(), selectedColumn()) += 1
If indexCalls <> 2 OrElse values(-2, 3) <> 12 Then End 2
values(-2, 3) = 100

Dim failure As Long
Dim handle As Any Ptr = ThreadCreate(@worker, @failure)
If handle = 0 Then End 3
ThreadWait(handle)
If failure <> 0 Then End 4
If values(-2, 3) <> 100 OrElse values(-1, 4) <> 22 Then End 5
If indexCalls <> 2 Then End 6
Print "THREAD_LOCAL_ARRAYS_PASS"

'' end of thread-local-arrays.bas
