'' examples/manual/fileio/for-append.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'APPEND'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgAppend
'' --------

Dim i As Integer
Dim file_number As Integer

For i = 1 To 10
	file_number = FreeFile

	If Open("test.txt" For Append As #file_number) <> 0 Then
		Print "Could not append to test.txt"
	Else
		Print #file_number, "extending test.txt"
		Close #file_number
	End If
Next
