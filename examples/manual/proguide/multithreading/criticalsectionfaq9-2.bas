'' examples/manual/proguide/multithreading/criticalsectionfaq9-2.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'Critical Sections FAQ'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ProPgMtCriticalSectionsFAQ
'' --------

'' Worker argument boundary:
''
'' Each worker receives only its encoded thread number 1 through 9. The
'' demonstration converts it through a pointer-sized integer before using it.

Sub Thread (ByVal p As Any Ptr)
	Dim As Integer threadNumber = CInt(CLngInt(p))
	Color threadNumber + 8, threadNumber
	For I As Integer = 1 To 50 - 2 * threadNumber
		Print " " & threadNumber & " ";
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

Screen 0
test()
Print "Any key to continue"
Sleep

Screen 12
test()
Print "Any key to quit"
Sleep
