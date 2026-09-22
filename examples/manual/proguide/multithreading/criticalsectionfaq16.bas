'' examples/manual/proguide/multithreading/criticalsectionfaq16.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'Critical Sections FAQ'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ProPgMtCriticalSectionsFAQ
'' --------

Const MAX_THREAD_COUNT As UInteger = 4

Type Thread
	Dim As UInteger valueIN
	Dim As Double valueOUT
	Dim As Any Ptr pHandle
	Declare Static Sub SumUpTo_1(ByVal pt As Thread Ptr)
	Declare Static Sub SumUpTo_2(ByVal pt As Thread Ptr)
End Type

Sub Thread.SumUpTo_1(ByVal pt As Thread Ptr)
	pt->valueOut = 0
	For I As UInteger = 1 To pt->valueIN
		pt->valueOUT += I
	Next I
End Sub

Sub Thread.SumUpTo_2(ByVal pt As Thread Ptr)
	Dim As Double value = 0
	For I As UInteger = 1 To pt->valueIN
		value += I
	Next I
	pt->valueOUT = value
End Sub

Sub MyThreads(ByVal pThread As Any Ptr, ByVal threadNB As UInteger = 1)
	If threadNB = 0 OrElse threadNB > MAX_THREAD_COUNT Then
		Print "Thread count must be between 1 and"; MAX_THREAD_COUNT
		Exit Sub
	End If

	'' The fixed four-slot local table is bounded before any thread state is stored.
	'' FB-LINTER: DISABLE-NEXT-LINE FBL524
	Dim As Thread td(1 To MAX_THREAD_COUNT)
	Dim As Double t

	t = Timer
	For i As Integer = 1 To threadNB
		td(i).valueIN = 100000000 + i
		td(i).pHandle = ThreadCreate(pThread, @td(i))
	Next I
	For i As Integer = 1 To threadNB
		ThreadWait(td(i).pHandle)
	Next I
	t = Timer - t

	For i As Integer = 1 To threadNB
		Print "   SumUpTo(" & td(i).valueIN & ") = " & td(i).valueOUT, _
			  "(right result : " & (100000000# + i) * (100000000# + i + 1) / 2 & ")"
	Next I
	Print "      total time : " & t & " s"
	Print
End Sub

Print
For i As Integer = 1 To 4
	Print "Each thread (in parallel) accumulating result directly in shared memory:"
	Mythreads(@Thread.SumUpTo_1, I)
	Print "Each thread (in parallel) accumulating result internally in its local memory:"
	Mythreads(@Thread.SumUpTo_2, I)
	Print "-----------------------------------------------------------------------------"
	Print
Next i

Sleep
