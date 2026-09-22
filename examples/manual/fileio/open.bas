'' examples/manual/fileio/open.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'OPEN'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgOpen
'' --------

' Create a string and fill it.
Dim buffer As String, f As Integer
buffer = "Hello World within a file."

' Find the first free file number.
f = FreeFile

' Open the file "file.ext" for binary usage, using the file number "f".
Open "file.ext" For Binary As #f
If Err>0 Then Print "Error opening the file":End

' Place our string inside the file, using number "f".
' file.ext contains exactly the bytes of buffer for this manual transfer.
' FB-LINTER: DISABLE-NEXT-LINE FBL-DOC-BIN-003
If Put(#f, , buffer) <> 0 Then Print "Error writing the file"

' Close all open files.
Close

' End the program. (Check the file "file.ext" upon running to see the output.)
End
