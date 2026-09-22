'' examples/manual/proguide/pointers/addsub.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'Pointer Arithmetic'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ProPgPtrArithmetic
'' --------

Dim p As Integer Ptr = New Integer[2]

If p = 0 Then
	Print "Memory allocation failed"
Else
	'' p was checked before assigning the two allocated elements.
	'' FB-LINTER: DISABLE-NEXT-LINE FBL-PTR-001
	*p = 1
	'' FB-LINTER: DISABLE-NEXT-LINE FBL-PTR-001
	*(p + 1) = 2
	Delete[] p
End If
