'' examples/manual/fileio/freefile-good.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'FREEFILE'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgFreefile
'' --------

Dim As Integer fr, fs
' The CORRECT way:
fr = FreeFile

If Open("File1" For Input As #fr) <> 0 Then
	Print "Could not open File1"
Else
	fs = FreeFile

	If Open("File2" For Input As #fs) <> 0 Then
		Print "Could not open File2"
	Else
		Close #fs
	End If

	Close #fr
End If
