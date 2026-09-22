'' examples/manual/fileio/freefile.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'FREEFILE'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgFreefile
'' --------

' Create a string and fill it.
Dim buffer As String, f As Integer
buffer = "Hello World within a file."

' Find the first free file number.
f = FreeFile

' Open the file "file.ext" for binary usage, using the file number "f".
If Open("file.ext" For Binary As #f) <> 0 Then
	Print "Could not open file.ext"
Else

	' file.ext contains exactly the bytes of buffer for this tutorial transfer.
	' FB-LINTER: DISABLE-NEXT-LINE FBL-DOC-BIN-003
	If Put(#f, , buffer) <> 0 Then Print "Could not write file.ext"

	' Close the file.
	Close #f
End If

' End the program. (Check the file "file.ext" upon running to see the output.)
End
