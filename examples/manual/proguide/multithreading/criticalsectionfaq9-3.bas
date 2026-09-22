'' examples/manual/proguide/multithreading/criticalsectionfaq9-3.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'Critical Sections FAQ'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ProPgMtCriticalSectionsFAQ
'' --------

'' Thread mutex synchronization:
''
'' Every Color call and console update is protected by mutex. The worker
'' argument carries only the documented small thread number 1 through 9, which
'' is converted through a pointer-sized integer before use.
'' FB-LINTER: DISABLE-NEXT-LINE FBL301
Dim Shared As Any Ptr mutex

Sub Thread (ByVal p As Any Ptr)
	Dim As Integer threadNumber = CInt(CLngInt(p))
	MutexLock(mutex)
	Dim As ULong c0 = Color(threadNumber + 8, threadNumber)
	Dim As ULong c = Color()
	Color(LoWord(c0), HiWord(c0))
	MutexUnlock(mutex)
	For I As Integer = 1 To 50 - 2 * threadNumber
		MutexLock(mutex)
		c0 = Color(LoWord(c), HiWord(c))
		Print " " & threadNumber & " ";
		Color(LoWord(c0), HiWord(c0))
		MutexUnlock(mutex)
		Sleep 20 * threadNumber, 1
	Next I
End Sub

Sub test ()
	Dim As Any Ptr p(1 To 9)
	Locate 1, 1
	For I As Integer = 1 To 9
		p(I) = ThreadCreate(@Thread, Cast(Any Ptr, I))
		Sleep 25, 1
	Next I
	For I As Integer = 1 To 9
		ThreadWait(p(I))
	Next I
	Locate 16, 1
End Sub

mutex = MutexCreate

Screen 0
test()
Print "Any key to continue"
Sleep

Screen 12
test()
Print "Any key to quit"
Sleep

MutexDestroy(mutex)
