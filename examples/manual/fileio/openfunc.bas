'' examples/manual/fileio/openfunc.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'OPEN'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgOpen
'' --------

'function version of OPEN
Dim file_number As Integer
file_number = FreeFile

If Open("file.ext" For Binary Access Read As #file_number) = 0 Then

	Print "Successfully opened file"

	'' ...

	Close #file_number

Else

	Print "Error opening file"

End If
