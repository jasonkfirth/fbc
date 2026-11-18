'' examples/manual/threads/threadcall.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'THREADCALL'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgThreadCall
'' --------

'' Threading using "ThreadCall"

Sub worker( ByRef id As String, ByVal tlock As Any Ptr, ByVal count As Integer )
	For i As Integer = 1 To count
		MutexLock tlock
		Print "thread " & id;
		Locate , 20
		Print i & "/" & count
		MutexUnlock tlock
	Next
End Sub

Dim tlock As Any Ptr = MutexCreate()
Dim id_a As String = "A", id_b As String = "B"
Dim a As Any Ptr = ThreadCall worker(id_a, tlock, 6)
Dim b As Any Ptr = ThreadCall worker(id_b, tlock, 4)
ThreadWait a
ThreadWait b
MutexDestroy tlock
Print "All done (and without Dim Shared!)"

'' end of examples/manual/threads/threadcall.bas
