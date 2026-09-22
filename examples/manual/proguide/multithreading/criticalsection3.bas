'' examples/manual/proguide/multithreading/criticalsection3.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'Critical Sections'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ProPgMtCriticalSections
'' --------

Type UDT
	Dim As Integer number
	Dim As Integer tempo
	Dim As Any Ptr pThread
	Dim As ULongInt count
	Static As Any Ptr pMutex(Any)
	Static As Integer numberMax
	Static As Integer quit
End Type
Dim As Any Ptr UDT.pMutex(Any)
Dim As Integer UDT.numberMax
Dim As Integer UDT.quit

Sub Counter (ByVal pt As UDT Ptr)
	With *pt
		Locate .number, .number, 0
		Sleep 5, 1
		.count += 1
		Print .count;
		Locate .number, 30 + .number, 0
	End With
End Sub

Sub Thread (ByVal p As Any Ptr)
	Dim As Integer quit
	Dim As UDT Ptr pUDT = p
	With *pUDT
		Do
			MutexLock(.pMutex(.number))
			Counter(pUDT)
			quit = .quit
			MutexUnlock(.pMutex((.number + 1) Mod (UDT.numberMax + 1)))
			Sleep .tempo, 1
		Loop Until quit = 1
	End With
End Sub


UDT.numberMax = 6
ReDim UDT.pMutex(UDT.numberMax)
'' The literal maximum establishes a nonempty mutex array before querying bounds.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-ARR-004
Dim As UDT u(LBound(UDT.pMutex) To UBound(UDT.pMutex))
For I As Integer = LBound(UDT.pMutex) To UBound(UDT.pMutex)
	u(I).number = i
	u(I).tempo = 100 + 15 * I - 95 * Sgn(I)
	UDT.pMutex(I) = MutexCreate
	MutexLock(UDT.pMutex(I))
Next I
'' The same completed ReDim establishes the first mutex slot used for unlock.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-ARR-004
MutexUnlock(UDT.pMutex(u(LBound(UDT.pMutex)).number))

Dim As Single t = Timer
For I As Integer = LBound(UDT.pMutex) + 1 To UBound(UDT.pMutex)
	u(I).pThread = ThreadCreate(@Thread, @u(I))
Next I

Dim As String s
Do
	MutexLock(UDT.pMutex(u(0).number))
		s = Inkey
		If s <> "" Then
			UDT.quit = 1
		End If
	MutexUnlock(UDT.pMutex((u(0).number + 1) Mod (UDT.numberMax + 1)))
	Sleep u(0).tempo, 1
Loop Until s <> ""

For I As Integer = LBound(UDT.pMutex) + 1 To UBound(UDT.pMutex)
	ThreadWait(u(I).pThread)
Next I
t = Timer - t

For I As Integer = LBound(UDT.pMutex) To UBound(UDT.pMutex)
	MutexDestroy(UDT.pMutex(I))
Next I
Dim As ULongInt c
For I As Integer = LBound(UDT.pMutex) + 1 To UBound(UDT.pMutex)
	c += u(I).count
Next I
Locate UDT.numberMax+2, 1
Print CULngInt(c / t) & " increments per second"

Sleep
