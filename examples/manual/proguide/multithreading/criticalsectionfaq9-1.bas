'' examples/manual/proguide/multithreading/criticalsectionfaq9-1.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'Critical Sections FAQ'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ProPgMtCriticalSectionsFAQ
'' --------

'' Thread mutex synchronization:
''
'' Every Locate, Print, and saved cursor state operation is protected by mutex.
'' The worker argument carries only the documented small thread number 1 through
'' 9, which is converted through a pointer-sized integer before use.
'' FB-LINTER: DISABLE-NEXT-LINE FBL301
Dim Shared As Any Ptr mutex

Sub Thread (ByVal p As Any Ptr)
	Dim As Integer threadNumber = CInt(CLngInt(p))
	MutexLock(mutex)
	Dim As Long l0 = Locate()
	Locate threadNumber, threadNumber
	Dim As Long l = Locate()
	Locate HiByte(LoWord(l0)), LoByte(LoWord(l0)), HiWord(l0)
	MutexUnlock(mutex)
	For I As Integer = 1 To 50 - 2 * threadNumber
		Sleep 20 * threadNumber, 1
		MutexLock(mutex)
		l0 = Locate()
		Locate HiByte(LoWord(l)), LoByte(LoWord(l)), HiWord(l)
		Print Str(threadNumber);
		l = Locate()
		Locate HiByte(LoWord(l0)), LoByte(LoWord(l0)), HiWord(l0)
		MutexUnlock(mutex)
	Next I
End Sub

Sub test ()
	Dim As Any Ptr p(1 To 9)
	For I As Integer = 1 To 9
		p(I) = ThreadCreate(@Thread, Cast(Any Ptr, I))
		Sleep 25, 1
	Next I
	For I As Integer = 1 To 9
		ThreadWait(p(I))
	Next I
End Sub

mutex = MutexCreate

Screen 0
test()
Locate 15, 1
Print "Any key to continue"
Sleep

Screen 12
test()
Locate 15, 1
Print "Any key to quit"
Sleep

MutexDestroy(mutex)
